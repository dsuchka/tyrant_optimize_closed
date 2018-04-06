# tyrant_optimize

## Building on Linux

Prereqs:

* make
* g++
* Boost
* git

For Ubunutu/Debian

1. Install required packages
  * `apt install g++ make git libboost-all-dev`
2. Get the lastest rev. of tuo
  * `git clone git://github.com/APN-Pucky/tyrant_optimize`
3. Compile!
  * `cd tyrant_optimize && make -f Makefile.linux`
4. Update the card data files
  * `./update_xml.sh`

Note: You'll need at least 1G of ram to compile tuo

## Running on Linux

1. Add the cards you own to `data/ownedcards.txt`.  See [Adding owned cards on Linux](#adding-owned-cards-on-linux) for more info
2. Run the sim for your deck against another deck

If you want to try and build an optimized deck from the list of owned
cards and commanders you have see
https://docs.google.com/document/d/1n0lncyqaqBv7hRS4R8Nsx5_SZ_0gXH9WyjBgyKPzwyo/edit
for more details (but for Windows)

### <a name="adding-owned-cards-on-linux"></a> Adding owned cards on Linux

The format of the owned cards file is:

```
<card name>-<card level> (<card copies number>)
<card name>-<card level> #<card copies number>
```

Example: If you have three copies of Apex, all of them level 2, you would write it like this:
```
Apex-2 (3)
```
or
```
Apex-2 #3
```

**Notes:**
* If you don’t specify a card level, it is assumed that card is at
maximum level. Always put a `-1` on cards that aren’t upgraded (ex.:
two stock Omegas must be written like this: `Omega-1 #2` )
* If you have one fully upgraded Omega, you don’t need to write `Omega-6`, `Omega` is enough.
* If you don’t specify a card copies number, it is assumed that you have only one copy from that card.




