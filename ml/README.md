# Requirements

1. Python 3.X with pip/Anaconda on Windows
2. Java 1.8 or higher



# Install

```
$ pip install tuo_ml
```

If you do not have administrator rights append `--user`/`--break-system-packages` to above command.

Alternatively, if some errors occur in later steps to install from source.





# Run it

```
tuo/data $ tuo_train --help
tuo/data $ tuo_train
```

The output should look similar to:

```
Loading database from database.yml...done
Parsing database...
100%|█████████████████████████████████████| 1705/1705 [00:20<00:00, 84.00it/s]
100%|███████████████████████████████████████████| 1/1 [00:20<00:00, 20.30s/it]
Training...
Win......Average Error: +-1.752% (+-0.027%)
Stall....Average Error: +-0.750% (+-0.017%)
Loss.....Average Error: +-1.225% (+-0.022%)
Points...Average Error: +-1.760 (+-0.031)
```

The uncertainty of the training is indicated.
You can copy the `*.pmml` files into your `data/` folder to use them in tuo now.

# Run it from source

If you want to define your own ML model follow these steps.

## Download

Download the source zip from github and unzip it. Or better clone the repo
```
$ git clone --recursive https://github.com/APN-Pucky/tyrant_optimize.git tuo
$ cd tuo/ml
```

## Install

```
tuo/ml $ poetry install 
```

## Modify train.py to your likings.

## Run it
```
tuo/data $ poetry run tuo_train
```
