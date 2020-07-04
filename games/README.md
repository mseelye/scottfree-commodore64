## Games  
Again, all of the games in the [games](games) directory belong to the original authors. Provided here for the sake of convenience.

#### Ghost King by Jason Compton  
* **[Ghost King](https://ifdb.tads.org/viewgame?id=pv6hkqi34nzn1tdy)** by [Jason Compton](http://twitter.com/jpcwrites) "Your father is dead and you're sure your uncle is responsible. You tried to tell your mother so. Instead of believing you, she married him. Now you're going to uncover the truth and set things right..."

#### Scott Adams Classic Adventures (SACA)  
* **adv01.dat**      [Adventureland](https://ifdb.tads.org/viewgame?id=dy4ok8sdlut6ddj7)
* **adv02.dat**      [Pirate Adventure (aka. Pirate's Cove)](https://ifdb.tads.org/viewgame?id=zya3mo3njj58hewi)
* **adv03.dat**      [Secret Mission(aka "Mission Impossible")](https://ifdb.tads.org/viewgame?id=89kxtet3vb9lzj87)
* **adv04.dat**      [Voodoo Castle](https://ifdb.tads.org/viewgame?id=ay2jy3sc3e6s9j4k)
* **adv05.dat**      [The Count](https://ifdb.tads.org/viewgame?id=89qmjvv4cb0z93t5)
* **adv06.dat**      [Strange Odyssey](https://ifdb.tads.org/viewgame?id=025dj4on6jr2c867)
* **adv07.dat**      [Mystery Fun House](https://ifdb.tads.org/viewgame?id=cj05ocxhay4dbrfs)
* **adv08.dat**      [Pyramid of Doom](https://ifdb.tads.org/viewgame?id=hew4c6rciycb6vog)
* **adv09.dat**      [Ghost Town](https://ifdb.tads.org/viewgame?id=rlxb5i0vjrnfr6x9)
* **adv10.dat**      [Savage Island, Part I](https://ifdb.tads.org/viewgame?id=wkaibkem4nxzo53y)
* **adv11.dat**      [Savage Island, Part II](https://ifdb.tads.org/viewgame?id=aqy6km542aq20jh4)
* **adv12.dat**      [The Golden Voyage](https://ifdb.tads.org/viewgame?id=4yo4je8dh53ug9qs)
#### Scott Adams later Adventures (plus a sample game)  
* **adv13.dat**      [Sorcerer of Claymorgue Castle](https://ifdb.tads.org/viewgame?id=11tnb08k1jov4hyl)
* **adv14a.dat**     [Return to Pirate's Isle](https://ifdb.tads.org/viewgame?id=rp9eibu02f9vp2sv)
* **adv14b.dat**     [Buckaroo Banzai](https://ifdb.tads.org/viewgame?id=m85x5yr0zbopyuyb)
* **sampler1.dat**   [Mini Adventure Sampler](https://ifdb.tads.org/viewgame?id=7nkd8ib4xbeqr7pm)
#### Scott Adams later Adventures Marvel(TM) Adventures (Questprobe series)  
* **quest1.dat**     [The Hulk](https://ifdb.tads.org/viewgame?id=4blbm63qfki4kf2p)
* **quest2.dat**     [Spiderman](https://ifdb.tads.org/viewgame?id=ngi8ox3s9gfcand2)

These Scott Adams original text Adventure games are still copyrighted by Scott Adams and are not Public Domain. They may be freely downloaded and enjoyed though.
Please refer to the original shares at [ifarchive.org](http://ifarchive.org/indexes/if-archiveXscott-adamsXgamesXscottfree.html) Look for AdamsGames.zip  

#### The 11 Mysterious Adventures by Brian Howarth:  
(c) 1981,82,83. All games written by Brian Howarth. #5-7 co-written by Wherner Barnes, #8 and #11 co-written by Cliff J. Ogden.  

* **1_baton.dat**                     [The Golden Baton](https://ifdb.tads.org/viewgame?id=v148gq1vx7leo8al)
* **2_timemachine.dat**               [The Time Machine](https://ifdb.tads.org/viewgame?id=g7h92i8ucy0sfll2)
* **3_arrow1.dat**                    [Arrow of Death Part 1](https://ifdb.tads.org/viewgame?id=g25uklrs45gj7e02)
* **4_arrow2.dat**                    [Arrow of Death Part 2](https://ifdb.tads.org/viewgame?id=fy9klru0b5swgnsz)
* **5_pulsar7.dat**                   [Escape from Pulsar 7](https://ifdb.tads.org/viewgame?id=tbrvwzmvgmmnavhl)
* **6_circus.dat**                    [Circus](https://ifdb.tads.org/viewgame?id=bdnprzz9zomlge4b)
* **7_feasibility.dat**               [Feasibility Experiment](https://ifdb.tads.org/viewgame?id=up2ak731a6h2quna)
* **8_akyrz.dat**                     [The Wizard of Akyrz](https://ifdb.tads.org/viewgame?id=u1kmutsdwp8uys1h)
* **9_perseus.dat**                   [Perseus and Andromeda](https://ifdb.tads.org/viewgame?id=gti5j0nqvvqqvnzo)
* **A_tenlittleindians.dat**          [Ten Little Indians](https://ifdb.tads.org/viewgame?id=z7ettlqezn4mcnng)
* **B_waxworks.dat**                  [Waxworks](https://ifdb.tads.org/viewgame?id=lkt6sm3mgarb02bo)


All of the commercially published Scott Adams-format games available in TRS-80 .dat format are expected to work on this build of ScottFree. Some modern games which play at the boundaries of the Adams specification may fail on this build due to memory limitations or other quirks.

### DAT files / ifarchive.org  
**DAT** Is a format used by some of the Scott Adams game kits and players. 
You can find a lot of them on sites like:  
http://ifarchive.org/indexes/if-archiveXscott-adamsXgames.html

### BDAT files  
**BDAT** Is a simple binary format based on the DAT format from ScottFree. I cobbled this together in an effort to decrease load times in ScottFree64. I will post a tool to convert DAT to BDAT and BDAT to DAT.  
Check the source for how it loads. To use a bdat file you just use the bdat instead of the dat file.  
Example:  
`RUN:REM -R -D GHOSTKING.BDAT`  

#### BDAT format  
The BDAt format is based on, and is only a condensed form of, the more standard DAT format. It very nearly has the absolute minimum to capture everything in the DAT file.  

Exceptions are:..
* **\r\n** - Carriage return (0x0D)and Linefeed (0x0A) characters are not optimized, and in some cases both wil be saved in the BDAT. These can likely be optimized to be just linefeeds.  
* **Null terminators** - All strings are prefixed with a length, so there is no technical need for a null terminator on the strings. They are there as it makes loading them slightly less code.  
* **Oversized datatypes** - I believe some of the datatypes could be single bytes, but are instead words.

Things "Missing":  
* There is no identifier before the header to identify the format, or the version of the format.  Since BDAT is only intended as an optimization for the C64, I don't think this is a problem.
* There are no checksums, this could also be a good addition, but would be overkill for this format as it is in no way an attempt at making any kind of a new standard.

#### Format Legend
* **hu** - Unsigned Short
* **l/h** - lo/hi-little endian

#### BDAT Header
It's important to note that some values in the DAT header are **zero based**, the code adds 1 to some of these values.  
```
00000000h: 00 00 27 00 93 00 3F 00 12 00 19 00 01 00 08 00 ; ..'.“.?.........
00000010h: 04 00 FF FF 62 00 12 00                         ; ..ÿÿb...

Byte#(dec) Example(hex) Data Type   Byte Order  Description
000-001    00 00        hu          l/h         DAT "Magic Number" - Usually 0x0000, although some Adventures have this set.
002-003    27 00        hu          l/h         Number of items, in this example 39 items.
004-005    93 00        hu          l/h         Number of actions, in this example 147 actions.
006-007    3F 00        hu          l/h         Number of word pairs, in this example 63 word pairs.
008-009    12 00        hu          l/h         Number of rooms, in this example 18 rooms.
010-011    19 00        hu          l/h         Max Carry, in this example 25 is the max carry.
012-013    01 00        hu          l/h         Player Start Room, in this example room #1.
014-015    08 00        hu          l/h         Number of treasures, in this example 8 treasures.
016-017    04 00        hu          l/h         Word Length, in this example word length is 4.
018-019    FF FF        hu          l/h         Light Time, in this example 65535, (or if it were signed -1), FFFF/-1 represents "unlimited".
020-021    62 00        hu          l/h         Number of messages, in this example 98 messages.
022-023    12 00        hu          l/h         Treasure Room, in this case the treasure room is room #18.
```
