import yaml
from smpl_io.io import sed
from yaml.loader import SafeLoader
import argparse
import tqdm
import pandas
from numpy import asarray, asmatrix
from numpy import mean
from numpy import std
from sklearn.datasets import make_regression
from sklearn.model_selection import cross_val_score
from sklearn.model_selection import RepeatedStratifiedKFold
from xgboost.sklearn import XGBRegressor
from sklearn.model_selection import RepeatedKFold
from sklearn2pmml import sklearn2pmml
from sklearn2pmml.pipeline import PMMLPipeline

base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

def hash_to_ids_ext_b64(hash, nmin=12):
    ids = []
    c = 0
    while c < len(hash):
        # for pc in hash:
        id = 0
        factor = 1
        p = base64_chars.find(hash[c])
        if p == -1:
            raise RuntimeError("Invalid hash character")
        d = p
        while d < 32:
            id += factor * d
            factor *= 32
            c = c + 1
            pc = hash[c]
            p = base64_chars.find(pc)
            if p == -1:
                raise RuntimeError("Invalid hash character")
            d = p
        id += factor * (d - 32)
        c = c + 1
        ids.append(id)
    while len(ids) < nmin:
        ids.append(0)
    return ids

def test_it(model, X, y,bool=True):
    cv = RepeatedKFold(n_splits=10, n_repeats=3, random_state=1)
    n_scores = cross_val_score(
        model,
        X,
        y,
        scoring="neg_mean_absolute_error",
        cv=cv,
        n_jobs=-1,
        error_score="raise",
    )
    pp = "%%" if bool else ""
    vmean = mean(n_scores) * 100 if bool else mean(n_scores)
    vstd = std(n_scores) * 100 if bool else std(n_scores)
    print("Average Error: +-%.3f"% ( -vmean) + pp +" (+-%.3f" + pp +")" % ( vstd))

def main():
    parser = argparse.ArgumentParser(
        prog="Machine Learning for TUO",
        description="Train a model for TUO",
        epilog="Author: Alexander Puck Neuwirth",
    )
    parser.add_argument(
        "-d", "--database", help="database file", type=str, default="database.yml"
    )
    parser.add_argument("-o", "--output", help="output directory", type=str, default=".")
    parser.add_argument(
        "-l",
        "--limit",
        help="minimum iterations needed to be included",
        type=int,
        default=10000,
    )
    args = parser.parse_args()

    print("Loading database...", end="", flush=True)
    db = yaml.load(sed("\t", "  ", args.database), Loader=SafeLoader)
    print("done")
    # print(db.keys())
    l = list(db.keys())[2:]
    # print(db[l[0]])


    xdata = []  # 10 + 10 ids each
    dic = None
    dics = []
    for ll in tqdm.tqdm(l, desc="Parsing data", position=0, leave=True):
        hx = hash_to_ids_ext_b64(ll)
        for k in tqdm.tqdm(db[ll].keys(), desc="Parsing data", position=1, leave=True):
            hk = hash_to_ids_ext_b64(k)
            for kk in db[ll][k]:
                # hfield = int(hashlib.sha256(ll.encode()).hexdigest(), 16) % (10 ** 9)

                # print(hfield)
                res = db[ll][k][kk].split(" ")
                if int(res[-1]) >= args.limit:
                    dic = {}
                    for i, v in enumerate(hx):
                        dic["f" + str(i)] = v
                    for i in range(0, len(hk)):
                        dic["y" + str(i)] = hk[i]
                    hkk = hash_to_ids_ext_b64(kk)
                    for i in range(0, len(hkk)):
                        dic["e" + str(i)] = hkk[i]
                    dic["w"] = float(res[0]) / float(res[-1])
                    dic["s"] = float(res[1]) / float(res[-1])
                    dic["l"] = float(res[2]) / float(res[-1])
                    dic["p"] = float(res[3]) / float(res[-1])
                    dics += [dic]
                    # df = df.append(dic,ignore_index=True)
                    # print(k,hash_to_ids_ext_b64(k), kk , hash_to_ids_ext_b64(kk), float(res[0]) / float(res[-1]), res[-1])

    df = pandas.DataFrame(dics)
    if dic is None:
        raise RuntimeError("No data found, consider decreasing the limit parameter (-l)")

    xdata = df[df.columns.difference(["w", "s", "l", "p"])]



    print("Training...")
    print("Win........", end="",flush=True)
    wpmml_pipeline = PMMLPipeline([("regressor", XGBRegressor())])
    wpmml_pipeline.fit(xdata, df["w"])
    wpmml_pipeline.configure(compact=False)
    sklearn2pmml(wpmml_pipeline, args.output + "/win.pmml")
    test_it(wpmml_pipeline, xdata, df["w"])


    print("Stall......", end="",flush=True)
    spmml_pipeline = PMMLPipeline([("regressor", XGBRegressor())])
    spmml_pipeline.fit(xdata, df["s"])
    spmml_pipeline.configure(compact=False)
    sklearn2pmml(spmml_pipeline, args.output + "/stall.pmml")
    test_it(spmml_pipeline, xdata, df["s"])

    print("Loss.......", end="",flush=True)
    lpmml_pipeline = PMMLPipeline([("regressor", XGBRegressor())])
    lpmml_pipeline.fit(xdata, df["l"])
    lpmml_pipeline.configure(compact=False)
    sklearn2pmml(lpmml_pipeline, args.output + "/loss.pmml")
    test_it(lpmml_pipeline, xdata, df["l"])

    print("Points.....", end="",flush=True)
    ppmml_pipeline = PMMLPipeline([("regressor", XGBRegressor())])
    ppmml_pipeline.fit(xdata, df["p"])
    ppmml_pipeline.configure(compact=False)
    sklearn2pmml(ppmml_pipeline, args.output + "/points.pmml")
    test_it(ppmml_pipeline, xdata, df["p"],False)


if __name__ == "__main__":
    main()