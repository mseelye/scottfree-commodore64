## BDAT format  
The BDAt format is based on, and is only a condensed form of, the more standard DAT format. It very nearly has the absolute minimum to capture everything in the DAT file.  

Exceptions are:..
* **\r\n** - Carriage return (0x0D)and Linefeed (0x0A) characters are not optimized, and in some cases both wil be saved in the BDAT. These can likely be optimized to be just linefeeds.  
* **Null terminators** - All strings are prefixed with a length, so there is no technical need for a null terminator on the strings. They are there as it makes loading them slightly less code.  
* **Oversized datatypes** - I believe some of the datatypes could be single bytes, but are instead words.
* **Escaped Quotation marks** - Some games use "" to escape a " in a message, room description. This should be removed before BDAT is created. (Might be a current bug. O_o)  

Things "Missing":  
* There is no identifier before the header to identify the format, or the version of the format.  Since BDAT is only intended as an optimization for the C64, I don't think this is a problem.  
* There are no checksums, this could also be a good addition, but would be overkill for this format as it is in no way an attempt at making any kind of a new standard.  

Note: For information on what all the DAT terms mean, and how these values are used in the game refer to Mike Taylor's fixes for the original **definition notes** from his [ScottKit](https://github.com/MikeTaylor/scottkit/tree/master/docs/notes) project.  

Change log:  
* 2020-07-04 - Initial Release!


#### Format Legend
* **hu** - Unsigned Short (2 bytes)
* **hhu** - Unsigned Char, for value (1 byte)
* **c** - Unsigned Char, for text (1 byte)
* **l/h** - lo/hi-little endian

#### BDAT Overall structure map
```
[Header 24 bytes]
[Action][0]
...actions: 16 bytes each...
[Action][n+1]
[WordPair][0]
...word pairs: (Each string length + 2) for each word pair...
[WordPair][n+1]

```

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

#### BDAT Actions
Actions are repeated "Number of Actions" + 1 times.  
Each "Action" is represented by 16 bytes. It has a Vocab, 5 Action-Conditions, and 2 Action-Actions.  (I didn't name these, don't blame me for Action Actions.)  

```
00000010h:                         84 03 12 34 56 78 9a bc ; ........„.......
00000020h: de f0 21 43 65 87 98 ba                         ; ....¬&..
(Note: these values are for illustration, not actual condition and action values.)  

Offset(dec)Example(hex) Data Type   Byte Order  Description
000-001    84 03        hu          l/h         Vocab (or what I call the "Phrase value"), in this case 900.
002-003    12 34        hu          l/h         Action Condition #0, 1 of 5
004-005    56 78        hu          l/h         Action Condition #1, 2 of 5
006-007    9a bc        hu          l/h         Action Condition #2, 3 of 5
008-009    de f0        hu          l/h         Action Condition #3, 4 of 5
010-011    21 43        hu          l/h         Action Condition #4, 5 of 5
012-013    65 87        hu          l/h         Action's Action #0, 1 of 2
014-015    98 ba        hu          l/h         Action's Action #1, 2 of 2
...
```

Since our header said 147, this block would be repeated 148 times, taking up 2368 bytes in the file.  

#### BDAT Word Pairs
Word pairs are a list of paired verb and nouns, the pairing seems to be just for the file format, and doesn't seem to relate to the use between the verb and noun in the game.  
Note: The first several sets of words seem to be in reserved spaces for directions, go, any, etc.  
Each word is usually 1-4(Word Length from header?) characters, sometimes with an extra character for a "synonym" asterisk.  
The word pairs are repeated "Number of word pairs" + 1 times. If a verb or noun "column" has less than it's pair, it will contain empty strings.  
The bytes used to store each word follow the pattern [length][byte*length][0].  
```
00000950h:                        |03 41 55 54 00|03 41 4E ; ....\+...AUT..AN
00000960h: 59 00|02 47 4F 00|04 4E 4F 52 54 00|05 2A 45 4E ; Y..GO..NORT..*EN
00000970h: 54 45 00|04 53 4F 55 54 00|05 2A 46 4F 4C 4C 00|; TE..SOUT..*FOLL.
00000980h: 04 45 41 53 54 00|04 2A 52 55 4E 00|            ; .EAST..*RUN.

Offset(dec)Example(hex) Data Type   Byte Order  Description
Verb:
000        03           hhu         n/a         Verb string length in bytes [0-255], in this case 3.
001-003    41 55 54     c           n/a         In this case the characters "AUT"
004        00           hhu         n/a         Null termination
Noun:
005        03           hhu         n/a         Noun Length in bytes [0-255], in this case 3.
006-008    41 4E 59     c           n/a         In this case the characters "ANY"
009        00           hhu         n/a         Null termination
...
```

Our header specified 63 word pairs so there are 63 + 1 pairs. The number of bytes will vary. The BDAT format does not store a value to skip over the pairs, the lengths must be traversed.  

#### BDAT Rooms
Rooms are 6 shorts (12 bytes), and a string (string length + 2). The string size varies so there is not a set size for each, like the word pairs they must be traversed.  
The 6 short values represent the room numbers for the directions north, south, east, west, up and down respectively.  
Not all rooms need to have exits as game code can move the player and items between rooms without them being connected.  
Connections can also be one-way, for example notice the hallyway's South goes to Room 1, but Room 1's north is 0. There is a different way to navigate from that room.  
The first room, #0, is usually all zeros (as in the example below), with a zero length string. However, this is not a rule as some games name the #0 room.  
```
00000c30h:                                       |00 00 00 ; ................
00000c40h: 00 00 00 00 00 00 00 00 00 00 00|00 00 00 00 00 ; ................
00000c50h: 00 00 00 00 00 00 00 14 2A 4D 79 20 70 72 69 6E ; ........*My prin
00000c60h: 63 65 6C 79 20 62 65 64 72 6F 6F 6D 00 07 00 01 ; cely bedroom....
00000c70h: 00 00 00 04 00 00 00 00 00 07 48 61 6C 6C 77 61 ; ..........Hallwa
00000c80h: 79                                              ; y...............

Offset(dec)Example(hex) Data Type   Byte Order  Description
000-001    00 00        hu          l/h         Room #0 North
002-003    00 00        hu          l/h         Room #0 South
004-005    00 00        hu          l/h         Room #0 East
006-007    00 00        hu          l/h         Room #0 West
008-009    00 00        hu          l/h         Room #0 Up
010-011    00 00        hu          l/h         Room #0 Down
012        00           hhu         n/a         Room #0 Description Text string length
013        00           hhu         n/a         Room #0 string Null termination

014-015    00 00        hu          l/h         Room #1 North
016-017    00 00        hu          l/h         Room #1 South
018-019    00 00        hu          l/h         Room #1 East
020-021    00 00        hu          l/h         Room #1 West
022-023    00 00        hu          l/h         Room #1 Up
024-025    00 00        hu          l/h         Room #1 Down
026        14           hhu         n/a         Room #1 Description Text string length (20 bytes)
027-046    2A...6D      c           n/a         Room #1 Description Text  "*My princely bedroom"
047        00           hhu         n/a         Room #1 string Null termination

048-049    07 00        hu          l/h         Room #2 North (Room 7)
050-051    01 00        hu          l/h         Room #2 South (Room 1)
052-053    00 00        hu          l/h         Room #2 East
054-055    04 00        hu          l/h         Room #2 West (Room 4)
056-057    00 00        hu          l/h         Room #2 Up
058-059    00 00        hu          l/h         Room #2 Down
060        07           hhu         n/a         Room #2 Description Text string length (7 bytes)
061-067    48...79      c           n/a         Room #2 Description Text  "Hallway"
068        00           hhu         n/a         Room #2 string Null termination
...
```

The header specified 18 rooms, so 18 + 1, 19 total. There would be 16 more room definitions after this.  

#### Messages
Messages are simply strings.  The first string tends to be an empty string (length 00 + 00 null terminator), but I don't think that is a rule either.  
String may contain carriage return and linefeed characters and quotation marks.

```
00000e70h:                         00 00|1A 44 72 69 6E 6B ;        ....Drink
00000e80h: 20 64 65 65 70 20 65 72 65 20 79 6F 75 20 64 65 ;  deep ere you de
00000e90h: 70 61 72 74 21 00|DA 57 65 6C 63 6F 6D 65 20 74 ; part!.ÚWelcome t
00000ea0h: 6F 20 47 48 4F 53 54 20 4B 49 4E 47 20 53 2E 41 ; o GHOST KING S.A

Offset(dec)Example(hex) Data Type   Byte Order  Description
000        00           hhu         n/a         Message #0 length in bytes [0-255], in this case 0.
                                                Note: this Message #0 is an empty string.
001        00           hhu         n/a         Message #0 null termination

002        1A           hhu         n/a         Message #1 length in bytes [0-255], in this case 26.
003-028    44...21      c           n/a         Message #1 "Drink deep ere you depart!"
029        00           hhu         n/a         Message #1 Null termination

030        DA           hhu         n/a         Message #2 length in bytes [0-255], in this case 218.
031-248    57...        c           n/a         Message #2 "Welcome to GHOST KING..." (truncated here)
249        00           hhu         n/a         Message #2 null termination (not shown above)
...
```

We have 99 total messages, including any empty strings, in this file since the header said 98.  

#### Items
Items are stored slightly differently in the BDAT format, but only to reduce the complexity of the load and save code.  
In a DAT file Items have a description text that can contain an "autoget" word at the end, notated with a slash before, or surrounding the autoget word.  
In the BDAT format it just stores 2 strings, and if the autoget string has a non-empty string it is used as the autoget word. No complex parsing needed.  
The loader code does free any empty autoget strings (releasing 2 bytes back to the heap) and sets the value in the data structure to NULL, as the code expects this.  
Items are a description string, an optional autoget string, and a starting room number.  The starting room number is also copied to a "InitialLoc" value which used in game logic, and to restart the game(in scottfree64).  

```
00002a30h:                      08 57 61 72 64 72 6F 62 65 ; ........Wardrobe
00002a40h: 00 00 00 01|06 2A 52 6F 62 65 2A 00 04 52 4F 42 ; .....*Robe*..ROB
00002a50h: 45 00 00|06 44 61 67 67 65 72 00 04 44 41 47 47 ; E...Dagger..DAGG
00002a60h: 00 00|                                          ; ................

Offset(dec)Example(hex) Data Type   Byte Order  Description
000        08           hhu         n/a         Item #0 Description length in bytes [0-255], in this case 8.
001-008    57...65      c           n/a         Item #0 Description "Wardrobe"
009        00           hhu         n/a         Item #0 Description null termination
010        00           hhu         n/a         Item #0 Autoget length in bytes [0-4?] (Might be "Word Length" from header?)
                                                Note: Item #0 does not have an Autoget word.
011        00           hhu         n/a         Item #0 Autoget null termination
012        01           hhu         n/a         Item #0 Starting Location (Room 1)

013        06           hhu         n/a         Item #1 Description length in bytes [0-255], in this case 6.
014-019    2A...2A      c           n/a         Item #1 Description "*Robe*" (Treasure!)
020        00           hhu         n/a         Item #1 Description null termination
021        04           hhu         n/a         Item #1 Autoget length in bytes [0-4?], 4 here.
022-025    52...45      c           n/a         Item #1 Autoget "ROBE"
026        00           hhu         n/a         Item #1 Autoget null termination
027        00           hhu         n/a         Item #1 Starting Location (Room 0 "Nowhere")

028        06           hhu         n/a         Item #2 Description length in bytes [0-255], in this case 6.
029-034    44...72      c           n/a         Item #2 Description "Dagger" (Pointy!)
035        00           hhu         n/a         Item #2 Description null termination
036        04           hhu         n/a         Item #2 Autoget length in bytes [0-4?], 4 here.
037-040    44...47      c           n/a         Item #2 Autoget "DAGG" (Dag, yo!)
041        00           hhu         n/a         Item #2 Autoget null termination
042        00           hhu         n/a         Item #2 Starting Location (Room 0 "Nowhere")
...
```

40 total items in the BDAT, as the header said 39 (plus one!).  

#### Comment Strings
Comment strings go along with the actions. There are "Number of Actions" + 1 comments in the BDAT file. Typically these seem to be empty strings.  
The ScottFree64 engine loads these and discards immediately, both for DAT and BDAT.  
The structure is identical to the Messages, with the exception that the count comes from "Number of Actions" + 1.
```
00002de0h:                                           00 00|; ................
00002df0h: 00 00|00 00|00 00|00 00|00 00|00 00|00 00|00 00|; ................
00002e00h: 00 00|00 00|00 00|00 00|00 00|00 00|00 00|00 00|; ................
00002e10h: 00 00|04 63 6F 6E 74 00|00 00|00 00|00 00|00 00|; ...cont.........

Offset(dec)Example(hex) Data Type   Byte Order  Description
000        00           hhu         n/a         Comment #0 length in bytes [0-255], in this case 0.
                                                Note: Comment #0 is an empty string.
001        00           hhu         n/a         Comment #0 null termination
...(skipping a bunch of empties like the above)...
Offset(dec)Example(hex) Data Type   Byte Order  Description
036        04           hhu         n/a         Comment #18 length in bytes [0-255], in this case 4.
037-040    63...74      c           n/a         Comment #18 "cont" (Short for "Continue", seen often in ScottKit DATs)
041        00           hhu         n/a         Comment #18 null termination
```

There are a total of 148 actions, and so there are 148 comments.  

#### Game Info
There are 3 shorts at the end of the BDAT file representing a version #, an Adventure #, and some "Magic Number".  
These are quickly displayed when the BDAT loads, but are otherwise not used in any game logic.  
The game displays the version value as a Major and Minor format: [value / 100].[value mod 100].  (107 would be "1.7")
```
00002f70h: -- -- 07 00 43 06 00 00                         ; ....C...

Offset(dec)Example(hex) Data Type   Byte Order  Description
000-001    07 00        hu          l/h         Version number for the Adventure. In this case 7, or "0.7"  
002-003    43 06        hu          l/h         Adventure number. In this case 1603 (t'was a good year)
004-005    00 00        hu          l/h         Another "Magic Number", usually 0.
```

