/*
 *  ScottFree-64 - a Reworking of ScottFree Revision 1.14b for the Commodore 64
 *  (C) 2020 - Mark Seelye / mseelye@yahoo.com
 *  Version 2.1.0
 *  Heavier cbm optimizations.
 *  Commodore 128 and 80 Column Mode
 *
 *  Requires: cc65 dev environment, build essentials (make)
 *  Optional: 
 *     Vice tooling for c1541 (making the disk images), petcat (basic stub)
 *
 *  build c64 version with:
 *    make clean all
 *
 *  build Commodore 128 version with:
 *    SYS=c128 make clean all
 *
 *  Run with:
 *    Your Commodore 64 or Commodore 128! (Or VICE, x64, x64sc, x128)
 *    Once you load the binary with 
 *      load "scottfree64",8,1
 *     Or on the Commodore 128:
 *      load "scottfree128",8,1
 *    You use cc65's argument passing like:
 *      run:rem -d myfavgame.dat mysavegame
 *    The BASIC stub has information on how to load run it.
 *    Hint:Cursor up to the load statement, press enter.
 *         After it loads, change the name of the .dat file for the run and press enter.
 *
 *  Notes:
 *      This is a reworking of the reworking (v1), it has heavier optimizations for memory for the c64.
 *      This has all the changes that v1 had, but also removes as much stdio as possible as 
 *      printf, scanf, etc. take a lot of memory.
 *      In addition to output (printf/sprintf), and file i/o (fscanf) I also created 
 *      an all asm input parser for verb and noun that takes much less space than having
 *      sscanf linked in.
 *      I've also made use of bank 1 on the Commodore 128, storing all the messages there to save memory in bank 0.
 *      Added prompt to Load last save on death, or Restart
 *      Rearranged messages in bank1 to be able to store messages up to 510 characters long.
 *
 *   Thanks:
 *      Big thanks for all the testing, advice and encouragement from Jason Compton, 
 *      Robin Harbron, Adrian Gonzalez (dW), Per (MagerValp), Oliver V.(Six), 
 *      Bob S.(Dokken), David (jbevren) and Sam.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  Original copyright and license for ScottFree Revision 1.14b follows:
 */
/*
 *    ScottFree Revision 1.14b
 *
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version
 *    2 of the License, or (at your option) any later version.
 *
 *
 *    You must have an ANSI C compiler to build this program.
 */

#include <string.h>         // strcpy, strncasecmp, memcpy, strchr, strcmp, strlen, strcasecmp
#include <stdlib.h>         // exit, malloc, rand, atol, free, _heapmemavail, srand
#include <conio.h>          // cgetc, clrscr, gotoxy, cclearxy, wherex, wherey, textcolor, bordercolor
#include <cbm.h>            // cbm_read, cbm_open, cbm_close
#include <unistd.h>         // sleep
#include <stdint.h>         // [u]int[8|16|32]_t (also included with one of the above^)

#include "scottfree64.h"    // various constants used for the c64/c128 versions
#include "scottfree.h"      // original SCOTT.H header file from 1.14b
#include "sf64.h"           // header file for optimized asm routines for c64/c128
#if defined(__C128__)
#include <em.h>             // cc65 extended memory driver for c128's bank 1
#endif

Header GameHeader;
Item *Items;
Room *Rooms;
uint8_t **Verbs;
uint8_t **Nouns;
Action *Actions;
// c128 stores page, offset, and length in bank 0 and messages in bank 1
#if defined(__C128__)
    MessageInfo *Messages;
#else
    uint8_t **Messages;
#endif

uint8_t LightRefill = 0;
uint8_t NounText[16];
uint8_t Counters[16];          // Range unknown
int16_t CurrentCounter = 0;
uint8_t SavedRoom = 0;
uint8_t RoomSaved[16];          // Range unknown
uint8_t Redraw = 0;             // Update item window
uint8_t Restart = 0;            // Flag to restart game, see NewGame()
uint8_t *SavedGame = NULL;      // Name of the saved game that was loaded, if any
uint8_t Options = 0;            // Option flags set
uint8_t Width = 0;              // Terminal width
uint8_t TopHeight = 0;          // Height of top window
uint8_t InitialPlayerRoom = 0;  // added to help with restarting w/o reloading see: NewGame()
uint8_t block[512];             // global buffer, used by multiple functions for temp storage
uint8_t OutputPos=0;            // Output position on line

// c128 shadow values so don't have to do VDC hoops in 80col mode
#if defined(__C128__)
    uint8_t currentTextcolor = COLOR_BLACK;
    uint8_t currentBorderColor = COLOR_BLACK;
    uint8_t currentBackgroundColor = COLOR_BLACK;
#endif

#define MyLoc    (GameHeader.PlayerRoom)

long BitFlags=0;    // Might be >32 flags, previous comment said, "I haven't seen >32 yet"

void Fatal(uint8_t *x) {
    print(x);
    print_char('\n');
    cgetc();
    exit(1);
}

void ClearScreen(void) {
    clrscr();
    gotoxy(0,0);
}

void ClearTop(void) {
    uint8_t ct=0;
    
    while(ct < TopHeight) {
        cclearxy (0, ct++, Width);
    }
}

void *MemAlloc(uint16_t size) {
    void *t = NULL;
    t = (void *)malloc(size);
    if(t==NULL) {
        print("malloc failed, size: ");
        print_number(size);
        Fatal(" Out of memory");
    }
    return(t);
}

// 1.14b steps random %: 1-4=%4 5-8:%7  9-12:%10 etc
// Updated to be more like ScottFree Java version 1.18.
uint8_t RandomPercent(uint8_t percent) {
    int16_t val = rand() % 100;
    // cc65's srand surpresses sign bit, checking anyway
    if(val < 0) {
        val = -val;
    }
    if (val >= (uint16_t)percent) {
        return(0);
    }
    return(1);
}

uint8_t CountCarried() {
    uint8_t ct=0;
    uint8_t n=0;
    while(ct<=GameHeader.NumItems)
    {
        if(Items[ct].Location==CARRIED)
            n++;
        ct++;
    }
    return(n);
}

uint8_t *MapSynonym(uint8_t *word) {
    uint8_t n=1;
    uint8_t *tp;
    static uint8_t lastword[16];     // Last non synonym
    while(n<=GameHeader.NumWords) {
        tp=Nouns[n];
        if(*tp=='*') {
            tp++;
        } else {
            strcpy(lastword,tp);
        }
        if(strncasecmp(word,tp,GameHeader.WordLength)==0) {
            return(lastword);
        }
        n++;
    }
    return(NULL);
}

int8_t MatchUpItem(uint8_t *text, uint8_t loc) {
    uint8_t *word=MapSynonym(text);
    uint8_t ct=0;
    
    if(word==NULL) {
        word=text;
    }
    
    while(ct<=GameHeader.NumItems) {
        if(Items[ct].AutoGet && (uint8_t)Items[ct].Location==loc &&
            strncasecmp(Items[ct].AutoGet,word,GameHeader.WordLength)==0) {
            return(ct);
        }
        ct++;
    }
    return(-1);
}

// read file until next whitespace, convert to 32 bit integer
// Note: Uses global block buffer
uint32_t cbm_read_next(uint8_t filenum) {
    int8_t c = 0;
    uint8_t ct=0;
    int8_t t=0;
    // Skip over initial ws
    do {
        t = cbm_read(filenum, &c, 1);
    } while(t > 0 && is_space(c));
    if(t<=0) {
        print_signed_8(t);
        Fatal(" Error reading data");
    }
    block[ct++]=c; // first usable character after any whitespace
    do {
        t = cbm_read(filenum, &c, 1);
        if(t <= 0 || c==' ' || c=='\n' || c=='\r') // ws = parse delim, c==EOF
            break;
        block[ct++]=c;
    } while(ct<31); // could really be 11
    block[ct]=0;
    return(atol(block));
}

// Note: Hack, last loaded string length stored here - cheap/easy length
uint16_t lstrlen = 0;

// Note: uses common global block buffer, instead of local tmp buffer
char* ReadString (uint8_t filenum, uint8_t ca2p) {
    uint16_t ct=0;      // counter
    uint8_t c=0, nc=0;  // character, next character
    uint8_t pbc = 0;    // pushback buffer, our "ungetch"
    int8_t t = 0;       // read result
    char* sp;           // string pointer

    do {
        // check pushback buffer and take from there, otherwise read
        if(pbc==0) {
            t = cbm_read(filenum, &c, 1);
        } else {
            c = pbc;
            pbc = 0;
        }
    } while(t > 0 && is_space(c));
    if(c!='"') {
        Fatal("Initial quote expected");
    }
    do {
        // check pushback buffer and take from there, otherwise read
        if(pbc==0) {
            t = cbm_read(filenum, &c, 1);
        } else {
            c = pbc;
            pbc = 0;
        }
        if(t<=0) { // cbm EOF or ERROR
            Fatal("EOF in string");
        }
        // if char is a quote, check if we have an escaped quote ("")
        if(c=='"') {
            // check pushback buffer and take from there, otherwise read
            if(pbc==0) {
                t = cbm_read(filenum, &nc, 1);
            } else {
                nc = pbc;
                pbc = 0;
            }
            // if next char is NOT a quote, put next char in pushback buffer
            if(nc!='"') {
                pbc=nc; // "ungetc(nc,f)" - store "next char" in pushback buffer
                break;
            }
        }
        if(c==0x60) { // '`' "grave" character used as quote
            c='"';
        }
#if defined(__c64__)
        if(c=='\n') {
            block[ct++]=c;
            c='\r';
        }
#endif
        // if caller indicated convert ascii to petscii
        if(ca2p==1) {
            c=a2p(c);
        }
        block[ct++]=c;
    } while(1);
    block[ct]=0;            // terminate string
    lstrlen = ct;
    sp=MemAlloc(ct+1);      // allocate memory for string
    memcpy(sp,block,ct+1);  // copy string from buffer into allocated memory
    return(sp);
}

void DebugMessage(uint8_t loud, uint8_t *msg, uint16_t num) {
    if(loud) {
        print_char('\n');
        print_number(num);
        print_char(' ');
        print(msg);
    }
}

// Load Binary DAT String ([length][bytes * length][0])
uint8_t *LoadString(filenum) {
    uint8_t *string;
    // uint16_t length = 0; // Note: using global "lstrlen" so length can easily/cheaply be preserved
    lstrlen = 0;
    cbm_read(filenum, &lstrlen, 1);
    // If length shows as 255, that means the next byte needs to be added to 255. (Max 510)
    if(lstrlen==255) {
        cbm_read(filenum, &lstrlen, 1);
        lstrlen += 255;
    }
    string=(uint8_t *)MemAlloc(lstrlen+1);
    cbm_read(filenum, string, lstrlen+1);
    return(string);
}

// attempt read first three text values from file
// if possible then likely a regular DAT file, otherwise likely BDAT
uint8_t DetectMode(uint8_t *filename) {
    uint8_t filenum = 1;
    uint8_t device=PEEK(CBM_CURRENT_DEVICE_NUMBER);
    uint16_t chk[3] = {0,0,0};
    if (cbm_open(filenum, device, CBM_READ, filename)) {
        print("Unable to load \"");
        print(filename);
        print_char('\"');
        Fatal("File Error");
    }
    // Detect ascii or binary
    // Close and reopen in LoadDatabase call
    chk[0] = cbm_read_next(filenum);
    chk[1] = cbm_read_next(filenum);
    chk[2] = cbm_read_next(filenum);
    cbm_close(filenum);
    return( (chk[0]==0 && chk[1]==0 && chk[2]==0) ? BDAT_MODE : DAT_MODE );
}

// Load Binary DAT or DAT File
void LoadDatabase(uint8_t *filename, uint8_t loud) {
    uint16_t ct=0;
    uint8_t filenum=1;
    uint8_t device=PEEK(CBM_CURRENT_DEVICE_NUMBER);
    uint8_t mode=0;
    uint8_t currPage = 0;
    uint8_t currOffset = 0;
    Action *ap;
    Room *rp;
    Item *ip;
    uint8_t *mp;

    mode = DetectMode(filename);
    cbm_open(filenum, device, CBM_READ, filename);

    if(mode == BDAT_MODE) {
        print("BDAT\n");
        cbm_read(filenum, &GameHeader.Unknown, sizeof(GameHeader));
    } else {
        print("DAT\n");
        GameHeader.Unknown=cbm_read_next(filenum);
        GameHeader.NumItems=cbm_read_next(filenum);
        GameHeader.NumActions=cbm_read_next(filenum);
        GameHeader.NumWords=cbm_read_next(filenum);
        GameHeader.NumRooms=cbm_read_next(filenum);
        GameHeader.MaxCarry=cbm_read_next(filenum);
        GameHeader.PlayerRoom=cbm_read_next(filenum);
        GameHeader.Treasures=cbm_read_next(filenum);
        GameHeader.WordLength=cbm_read_next(filenum);
        GameHeader.LightTime=cbm_read_next(filenum);
        GameHeader.NumMessages=cbm_read_next(filenum);
        GameHeader.TreasureRoom=cbm_read_next(filenum);
    }
    InitialPlayerRoom=GameHeader.PlayerRoom;
    LightRefill=GameHeader.LightTime;

    Actions=(Action *)MemAlloc(sizeof(Action)*(GameHeader.NumActions+1));
    Verbs=(uint8_t **)MemAlloc(sizeof(uint8_t *)*(GameHeader.NumWords+1));
    Nouns=(uint8_t **)MemAlloc(sizeof(uint8_t *)*(GameHeader.NumWords+1));
    Rooms=(Room *)MemAlloc(sizeof(Room)*(GameHeader.NumRooms+1));
    Items=(Item *)MemAlloc(sizeof(Item)*(GameHeader.NumItems+1));

#if defined(__C128__)
    Messages=(MessageInfo *)MemAlloc(sizeof(MessageInfo)*(GameHeader.NumMessages+1));
#else
    Messages=(uint8_t **)MemAlloc(sizeof(uint8_t *)*(GameHeader.NumMessages+1));
#endif

    ct=0;
    ap=Actions;
    DebugMessage(loud, "actions", GameHeader.NumActions+1);
    while(ct < GameHeader.NumActions+1) {
        if(mode==BDAT_MODE) {
            cbm_read(filenum, ap, sizeof(Action));
        } else {
            ap->Vocab=(short)cbm_read_next(filenum);
            ap->Condition[0]=(short)cbm_read_next(filenum);
            ap->Condition[1]=(short)cbm_read_next(filenum);
            ap->Condition[2]=(short)cbm_read_next(filenum);
            ap->Condition[3]=(short)cbm_read_next(filenum);
            ap->Condition[4]=(short)cbm_read_next(filenum);
            ap->Action[0]=(short)cbm_read_next(filenum);
            ap->Action[1]=(short)cbm_read_next(filenum);
        }
        ct++;
        ap++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    DebugMessage(loud, "pairs", GameHeader.NumWords+1);
    while(ct < GameHeader.NumWords+1) {
        if(mode==BDAT_MODE) {
            Verbs[ct]=LoadString(filenum);
            Nouns[ct]=LoadString(filenum);
        } else {
            Verbs[ct]=ReadString(filenum,0);
            Nouns[ct]=ReadString(filenum,0);
        }
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    rp=Rooms;
    DebugMessage(loud, "rooms", GameHeader.NumRooms+1);
    while(ct < GameHeader.NumRooms+1) {
        if(mode==BDAT_MODE) {
            cbm_read(filenum, rp->Exits, 6 * sizeof(uint16_t));
            rp->Text = LoadString(filenum);
            a2p_string(rp->Text);
        } else {
            rp->Exits[0]=(short)cbm_read_next(filenum);
            rp->Exits[1]=(short)cbm_read_next(filenum);
            rp->Exits[2]=(short)cbm_read_next(filenum);
            rp->Exits[3]=(short)cbm_read_next(filenum);
            rp->Exits[4]=(short)cbm_read_next(filenum);
            rp->Exits[5]=(short)cbm_read_next(filenum);
            rp->Text=ReadString(filenum,1);
        }
        ct++;
        rp++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    DebugMessage(loud, "messages", GameHeader.NumMessages+1);
    while(ct < GameHeader.NumMessages+1) {
        if(mode==BDAT_MODE) {
            mp=LoadString(filenum);
            a2p_string(mp);
        } else {
            mp=ReadString(filenum,1);
        }
#if defined(__C128__)
        {
            struct em_copy copyinfo;
            Messages[ct].page = currPage;
            Messages[ct].offset = currOffset;
            Messages[ct].length = lstrlen + 1;
            copyinfo.page  = currPage;
            copyinfo.offs  = currOffset;
            copyinfo.count = lstrlen + 1;
            copyinfo.buf   = mp;
            em_copyto (&copyinfo);
            // page math: Move page forward for each set of 255 crossed
            currPage += ((currOffset + lstrlen + 1) / 255);
            currOffset = currOffset + lstrlen + 1; // currOffset is 8 bit, wraps around
            free(mp); // Message stored in em now, free bank 0 buffer
        }
#else
        Messages[ct]=mp;
#endif
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    ip=Items;
    DebugMessage(loud, "items", GameHeader.NumItems+1);
    while(ct < GameHeader.NumItems+1) {
        if(mode==BDAT_MODE) {
            ip->Text=LoadString(filenum);
            ip->AutoGet=LoadString(filenum);
            // SF likes Autoget to be NULL not empty
            if (ip->AutoGet[0]==0) {
                free(ip->AutoGet);
                ip->AutoGet = NULL;
            }
            cbm_read(filenum, &ip->Location, 1);
        } else {
            uint8_t lo=0;
            ip->Text=ReadString(filenum,0);
            ip->AutoGet=strchr(ip->Text,'/');
            // Some games use // to mean no auto get/drop word!
            if(ip->AutoGet && strcmp(ip->AutoGet,"//") && strcmp(ip->AutoGet,"/*")) {
                uint8_t *t;
                *ip->AutoGet++=0;
                t=strchr(ip->AutoGet,'/');
                if(t!=NULL) {
                    *t=0;
                }
            }
            lo=cbm_read_next(filenum);
            ip->Location=(unsigned char)lo;
        }
        ip->InitialLoc=ip->Location;
        ct++;
        ip++;
        if(loud) {
            print_char('.');
        }
    }

    // Discard Comment Strings
    ct=0;
    DebugMessage(loud, "comments", GameHeader.NumActions+1);
    while(ct < GameHeader.NumActions+1) {
        if(mode==BDAT_MODE) {
            free(LoadString(filenum));
        } else {
            free(ReadString(filenum,0));
        }
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    if(loud) {
        uint16_t adv = 0;
        uint16_t v = 0;
        if(mode==BDAT_MODE) {
            cbm_read(filenum, &v, 2);
            cbm_read(filenum, &adv, 2);
        } else {
            adv=cbm_read_next(filenum);
            v=cbm_read_next(filenum);
        }
        print("\nv");
        print_number(v/100);
        print_char('.');
        print_number(v%100);
        print(" of Adv. ");
        print_number(adv);
        // skip magic number
        print("\nDone!\n");
    }

    cbm_close(filenum);
}

void clreol() {
    uint8_t x=0, y=0;
    x=wherex();
    y=wherey();
    cclearxy(x, y, Width-x);
    gotoxy(x,y);
}

void OutReset()
{
    OutputPos=0;
    gotoxy(0,wherey()); // not 1
    clreol();
}

// note: can't use global block buffer, would overlap from Output()
void OutBuf(uint8_t *buffer) {
    uint8_t word[40];  // max "word" length
    uint8_t wp;

    while(*buffer) {
        if(OutputPos==0) {
            while(*buffer && is_space(*buffer)) {
                if(*buffer=='\n') {
                    print(EOL);
                    OutputPos=0;
                }
                buffer++;
            }
        }
        if(*buffer==0) {
            return;
        }
        wp=0;
        while(*buffer && !is_space(*buffer) && wp < 40) {
            word[wp++]=*buffer++;
        }
        word[wp]=0;
        if(OutputPos+strlen(word)>(Width-2)) {
            print(EOL);
            OutputPos=0;
        } else {
            gotoxy(OutputPos+0, wherey());
        }
        print(word);
        OutputPos+=strlen(word);
        if(*buffer==0) {
            return;
        }
        if(*buffer=='\n' || *buffer=='\r') {
            print(EOL);
            OutputPos=0;
        } else {
            OutputPos++;
            if(OutputPos<(Width-1)) {
                print(" ");
            }
        }
        buffer++;
    }
}

// output petscii instead of raw ascii, mode: 0-OutBuf, 1-print
// Note: Uses global block buffer
void OutputPetscii(char* text, uint8_t mode) {
    uint8_t ct = 0;

    strcpy(block, text);
    do {
        if(text[ct]==0) {
            break;
        }
        block[ct] = a2p(block[ct]);
        ct++;
    } while(1);
    // Use OutBuf or just print, OutBuf handles word wrapping
    if(mode==0) {
        OutBuf(block);
    } else {
        print(block);
    }
}

// Note: Uses global block buffer
void Output(char* text) {
    strcpy(block, text);
    OutBuf(block);
}

void OutputNumber(uint16_t num) {
    OutBuf((uint8_t *)bufnum16(num)); // Note: doesn't print signed
}

// Note: Uses global block buffer
void OutputMessage(uint16_t ct) {
#if defined(__C128__)
    // copy message down from bank 1 and call OutBuf
    struct em_copy copyinfo;
    copyinfo.page  = Messages[ct].page;
    copyinfo.offs  = Messages[ct].offset;
    copyinfo.count = Messages[ct].length;
    copyinfo.buf   = block;
    em_copyfrom (&copyinfo);
    OutBuf(block);
#else
    Output(Messages[ct]);
#endif
}

// Handle output room description/state to "top area"
void Look(uint8_t cs) {
    static uint8_t *ExitNames[6]= {
        "North","South","East","West","Up","Down"
    };
    Room *r = NULL;
    uint8_t ct = 0;
    uint8_t f = 0;
    uint8_t pos = 0;
    uint8_t xp = 0,yp = 0;

#if defined(__C128__)
    // c128 will push text down if any of this top text goes past the end of line
    // disable screen scroll while printing to top area.
    // Thanks TFM!
    uint8_t f8;
    f8 = PEEK(SCROLL);      // Save value
    POKE(SCROLL, f8 | 64);  // disable scroll
#endif

    // if clear screen is set, save x/y cursor for later, then clear
    if(cs == 1) {
        xp = wherex();
        yp = wherey();
        if(yp < TopHeight +1) {
            yp = TopHeight +1;
        }
        ClearTop();
        gotoxy(0,1); // output 1 down as the c64 will move it up one
    }

    if((BitFlags&(1L<<DARKBIT)) && Items[LIGHT_SOURCE].Location!= CARRIED
            && Items[LIGHT_SOURCE].Location!= MyLoc) {
        if(Options&YOUARE) {
            print("You can't see. It is too dark!");
            print(EOL);
        } else {
            print("I can't see. It is too dark!");
            print(EOL);
        }
        gotoxy(xp,yp);
#if defined(__C128__)
        POKE(SCROLL, f8); // restore bit 6 of $f8 (logical line)
#endif
        return;
    }
    r=&Rooms[MyLoc];
    if(*r->Text=='*')
    {
        print(r->Text+1);
        print(EOL);
    }
    else
    {
        if(Options&YOUARE) {
            print("You are ");
            print(r->Text);
            print(EOL);
        } else {
            print("I'm in a ");
            print(r->Text);
            print(EOL);
        }
    }
    ct=0;
    f=0;
    print(EOL);
    print("Obvious exits: ");
    while(ct<6) {
        if(r->Exits[ct]!=0) {
            if(f==0) {
                f=1;
            } else {
                print(", ");
            }
            print(ExitNames[ct]);
        }
        ct++;
    }
    if(f==0) {
        print("none");
    }
    print(".");
    print(EOL);
    ct=0;
    f=0;
    pos=0;
    while(ct<=GameHeader.NumItems) {
        if(Items[ct].Location==MyLoc) {
            if(f==0) {
                if(Options&YOUARE) {
                    print(EOL);
                    print("You can also see: ");
                } else {
                    print(EOL);
                    print("I can also see: ");
                }
                pos=16;
                f++;
            } else {
                print(" - ");
                pos+=3;
            }
            if(pos+strlen(Items[ct].Text)>(Width-5)) { // was 10
                pos=0;
                print(EOL);
            }
            OutputPetscii(Items[ct].Text, 1);
            pos += strlen(Items[ct].Text);
        }
        ct++;
    }
    clreol();
    print(EOL);
    clreol();
#if defined(__C128__)
    POKE(SCROLL, f8); // restore bit 6 of $f8 (logical line)
#endif
    if(cs == 1) {
        gotoxy(xp,yp); // put cursor back
    }
}

int8_t WhichWord(uint8_t *word, uint8_t **list) {
    uint8_t n=1;
    uint8_t ne=1;
    uint8_t *tp;

    while(ne<=GameHeader.NumWords) {
        tp=list[ne];
        if(*tp=='*') {
            tp++;
        } else {
            n=ne;
        }
        if(strncasecmp(word,tp,GameHeader.WordLength)==0) {
            return(n);
        }
        ne++;
    }
    return(-1);
}

// SetColors and Color Change for the C128
#if defined(__C128__)
void SetColors(uint8_t text, uint8_t border, uint8_t bg) {
    currentTextcolor = text & 15;
    currentBorderColor = border & 15;
    currentBackgroundColor = bg & 15;
    textcolor(currentTextcolor);
    bordercolor (currentBorderColor);
    bgcolor(currentBackgroundColor);
}

// Change text, border/bg colors on the C128
void ColorChange(uint8_t code) {
    uint8_t curT = currentTextcolor;
    uint8_t curD = currentBorderColor;
    uint8_t curB = currentBackgroundColor;
    int8_t sh = 0;
    sh = (code >= CH_F2)?-1:1; // Detect if shift pressed = -1, no shift = 1; F2,4,6,8 are > F1,3,5,7

    switch (code) {
        case CH_F1:
        case CH_F2:
            curT = (curT + sh) & 15;
            if(curT == curB) {
                curT += sh;
            }
            break;
        case CH_F3:
        case CH_F4:
            curD += sh;
            // 80 col mode no border - change bg
            if((PEEK(C128_MODE) & C128_MODE_80COL) == C128_MODE_80COL) {
                ColorChange(++code);
                return; // we don't want to recurse set
            }
            break;
        case CH_F5:
        case CH_F6:
            curB = (curB + sh) & 15;
            if(curT == curB) {
                curB += sh;
            }
            break;
        case CH_F7:
            curT = COLOR_BLACK;
            curD = COLOR_GRAY3;
            curB = COLOR_GRAY3;
            break;
        case CH_F8:
            curT = rand();
            curD = rand();
            curB = rand();
            // don't make text disappear
            if(curT == curB) {
                curT++;
            }
            break;
    }
    SetColors(curT, curD, curB);
    Look(1);
}

#elif defined(__C64__)

// Change text, border/bg colors on the C64
void ColorChange(uint8_t code) {
    uint8_t curT = textcolor(COLOR_BLACK); // get Current Text Color
    uint8_t curD = PEEK(VIC.bordercolor); // get Current Border Color $d020
    uint8_t curB = PEEK(VIC.bgcolor0); // get current BG Color $d021
    int8_t sh = 0;
    sh = (code >= CH_F2)?-1:1; // Shift pressed = -1, no shift = 1

    switch (code) {
        case CH_F1:
        case CH_F2:
            curT = (curT + sh) & 15;
            if(curT == curB) {
                curT += sh;
            }
            break;
        case CH_F3:
        case CH_F4:
            curD += sh;
            break;
        case CH_F5:
        case CH_F6:
            curB = (curB + sh) & 15;
            if(curT == curB) {
                curB += sh;
            }
            break;
        case CH_F7:
            curT = COLOR_BLACK;
            curD = COLOR_GRAY1;
            curB = COLOR_GRAY1;
            break;
        case CH_F8: // F8
            curT = rand();
            curD = rand();
            curB = rand();
            if(curT == curB) {
                curT++;
            }
            break;
    }
    textcolor(curT & 15);
    bordercolor (curD & 15);
    bgcolor(curB & 15);
    Look(1);
}
#endif

void LineInput(uint8_t *buf, uint8_t max) {
    uint8_t pos=0;
    uint8_t ch;
    
    while(1) {
        ch=(uint8_t)cgetc();
        if(pos >= max-1) {
            ch=13; // auto enter at max
        }
        switch(ch) {
            case 10:;
            case 13:;
                buf[pos]=0;
                print(EOL);
                return;
            case 8:;
            case 20:; // c64/c128 DEL key
            case 127:;
                if(pos>0) {
                    print("\010");
                    pos--;
                }
                break;
            default:
                if(ch >= CH_F1 && ch <= CH_F8) { // c64/c128 Check for Fkeys
                     ColorChange(ch);
                }
                if(ch >= ' ' && ch <= 126) {
                    buf[pos++]=ch;
                    print_char(ch);
                }
                break;
        }
    }
}

// Note: Can't use global block buffer, also reduced to 80 characters
void GetInput(int8_t* vb, int8_t* no) {
    uint8_t buf[80];
    uint8_t verb[10],noun[10];
    int8_t vc=0,nc=0;
    int8_t num=0;
    do {
        do {
            Output("\nTell me what to do ? ");
            Look(1); // hack: refresh the top here after c64 scroll, etc.
            LineInput(buf, 80);
            OutReset();
            num=parse_verb_noun(buf, 9, verb, noun); // "sscanf"
        } while(num<=0||*buf=='\n');
        if(num==1) {
            *noun=0;
        }
        if(*noun==0 && strlen(verb)==1) {
            // instead of using isupper/tolower, just expand switch cases
            switch(*verb) {
                case 'N':
                case 'n':strcpy(verb,"NORTH");break;
                case 'E':
                case 'e':strcpy(verb,"EAST");break;
                case 'S':
                case 's':strcpy(verb,"SOUTH");break;
                case 'W':
                case 'w':strcpy(verb,"WEST");break;
                case 'U':
                case 'u':strcpy(verb,"UP");break;
                case 'D':
                case 'd':strcpy(verb,"DOWN");break;
                case 'I':
                case 'i':strcpy(verb,"INVENTORY");break; // Brian Howarth interpreter also supports i
            }
        }
        nc=WhichWord(verb,Nouns);
        /* The Scott Adams system has a hack to avoid typing 'go' */
        if(nc>=1 && nc <=6) {
            vc=1;
        } else {
            vc=WhichWord(verb,Verbs);
            nc=WhichWord(noun,Nouns);
        }
        *vb = vc;
        *no = nc;
        if(vc==-1) {
            Output("\"");
            Output(verb);
            Output("\" is a word I don't know...sorry!\n");
        }
    } while(vc==-1);
    strcpy(NounText,noun); // Needed by GET/DROP hack
}

void cbm_write_value(uint8_t filenum, uint32_t value, uint8_t *end) {
    uint8_t *num;
    num = (uint8_t *)bufnum32(value);
    cbm_write(filenum, (uint8_t *)num, num[11]); // hack buffer has size at + 11 :/
    cbm_write(filenum, end, 1);
}

// Note: Can't use global block buffer, used to save last saved game file name
void SaveGame() {
    // Note: static variables maintain values, sloppy cheat to keep saved game name
    uint8_t filename[16];
    uint8_t ct=0;
    uint8_t filenum=1;
    uint8_t device=PEEK(CBM_CURRENT_DEVICE_NUMBER);
    uint8_t *sp=" ";
    uint8_t *cr="\n";
    Output("Filename: ");
    LineInput(filename, 16);
    Output("\n");
    if (cbm_open(filenum, device, CBM_WRITE, filename)) {
        Output("Unable to create save file.\n");
        return;
    }
    while(ct < 16) {
        cbm_write_value(filenum, Counters[ct], sp);
        cbm_write_value(filenum, RoomSaved[ct], cr);
        ct++;
    }
    cbm_write_value(filenum, BitFlags, sp);
    cbm_write_value(filenum, ((BitFlags&(1L<<DARKBIT))?1:0), sp);
    cbm_write_value(filenum, MyLoc, sp);
    cbm_write_value(filenum, CurrentCounter, sp);
    cbm_write_value(filenum, SavedRoom, sp);
    if(GameHeader.LightTime < 0) {
        cbm_write(filenum, "-1\n", 3);
    } else {
        cbm_write_value(filenum, GameHeader.LightTime, cr);
    }
    
    ct=0;
    while(ct <= GameHeader.NumItems) {
        cbm_write_value(filenum, Items[ct].Location, cr);
        ct++;
    }
    cbm_close(filenum);

    SavedGame = filename;

    Output("Saved.\n");
}

void LoadGame(uint8_t *name) {
    uint8_t ct=0;
    uint8_t filenum=1;
    uint8_t device=PEEK(CBM_CURRENT_DEVICE_NUMBER);
    uint8_t DarkFlag;

    SavedGame = name;
    print("\nLoading saved game '");
    print(SavedGame);
    print("'..");

     if (cbm_open(filenum, device, CBM_READ, name)) {
         Output("Unable to restore game.\n");
         return;
     }

    while(ct < 16) {
        Counters[ct] = cbm_read_next(filenum);
        RoomSaved[ct] = cbm_read_next(filenum);
        ct++;
    }
    BitFlags = cbm_read_next(filenum);
    DarkFlag = cbm_read_next(filenum);
    MyLoc = cbm_read_next(filenum);
    CurrentCounter = cbm_read_next(filenum);
    SavedRoom = cbm_read_next(filenum);
    GameHeader.LightTime = (int8_t)cbm_read_next(filenum);

    /* Backward compatibility */
    if(DarkFlag) {
        BitFlags|=(1L<<15);
    }
    ct=0;
    while(ct <= GameHeader.NumItems) {
        Items[ct].Location=(unsigned char)cbm_read_next(filenum);;
        ct++;
    }
    cbm_close(filenum);
}

// This restores state to the initial state so the 
// player can play again without reloading.
// Only needed to add InitialPlayerRoom.
void FreshGame() {
    uint8_t ct=0;

    // Clean up counters and room saved
    while(ct < 16) {
        Counters[ct]=0;
        RoomSaved[ct]=0;
        ct++;
    }

    BitFlags = 0;
    GameHeader.PlayerRoom = InitialPlayerRoom;
    CurrentCounter = 0;
    SavedRoom = 0;
    GameHeader.LightTime = LightRefill;
    ct=0;
    while(ct < GameHeader.NumItems+1) {
        Items[ct].Location=Items[ct].InitialLoc;
        ct++;
    }
}

// This determines if we should load a saved game again
// or start a fresh game again.
void NewGame(uint8_t ch) {
    ClearScreen();
    if( SavedGame == NULL || ch == 'r' || ch == 'R' ) {
        FreshGame();
    } else {
        LoadGame(SavedGame);
    }
}

uint8_t PerformLine(uint16_t ct) {
    uint8_t continuation=0;
    uint8_t pptr=0;
    uint16_t cc=0;
    uint16_t act[4];
    uint8_t param[5];
    while(cc<5) {
        uint16_t cv,dv;
        cv=Actions[ct].Condition[cc];
        dv=cv/20;
        cv%=20;
        switch(cv) {
            case ADD_ARG_TO_PARAMS:
                param[pptr++]=dv;
                break;
            case ITEM_CARRIED:
                if(Items[dv].Location!=CARRIED) {
                    return(0);
                }
                break;
            case ITEM_IN_ROOM_WITH_PLAYER:
                if(Items[dv].Location!=MyLoc) {
                    return(0);
                }
                break;
            case ITEM_CARRIED_OR_IN_ROOM_WITH_PLAYER:
                if(Items[dv].Location!=CARRIED&&
                    Items[dv].Location!=MyLoc) {
                    return(0);
                }
                break;
            case PLAYER_IN_ROOM:
                if(MyLoc!=dv) {
                    return(0);
                }
                break;
            case ITEM_NOT_IN_ROOM_WITH_PLAYER:
                if(Items[dv].Location==MyLoc) {
                    return(0);
                }
                break;
            case ITEM_NOT_CARRIED:
                if(Items[dv].Location==CARRIED) {
                    return(0);
                }
                break;
            case PLAYER_NOT_IN_ROOM:
                if(MyLoc==dv) {
                    return(0);
                }
                break;
            case BIT_FLAG_IS_SET:
                if((BitFlags&(1L<<dv))==0) {
                    return(0);
                }
                break;
            case BIT_FLAG_IS_CLEARED:
                if(BitFlags&(1L<<dv)) {
                    return(0);
                }
                break;
            case SOMETHING_CARRIED:
                if(CountCarried()==0) {
                    return(0);
                }
                break;
            case NOTHING_CARRIED:
                if(CountCarried()) {
                    return(0);
                }
                break;
            case ITEM_NOT_CARRIED_NOR_IN_ROOM_WITH_PLAYER:
                if(Items[dv].Location==CARRIED||Items[dv].Location==MyLoc) {
                    return(0);
                }
                break;
            case ITEM_IS_IN_GAME:
                if(Items[dv].Location==0) {
                    return(0);
                }
                break;
            case ITEM_IS_NOT_IN_GAME:
                if(Items[dv].Location) {
                    return(0);
                }
                break;
            case CURRENT_COUNTER_LESS_THAN_OR_EQUAL_TO:
                if(CurrentCounter>dv) {
                    return(0);
                }
                break;
            case CURRENT_COUNTER_GREATER_THAN:
                if(CurrentCounter<=dv) {
                    return(0);
                }
                break;
            case ITEM_IN_INITIAL_ROOM:
                if(Items[dv].Location!=Items[dv].InitialLoc) {
                    return(0);
                }
                break;
            case ITEM_NOT_IN_INITIAL_ROOM:
                if(Items[dv].Location==Items[dv].InitialLoc) {
                    return(0);
                }
                break;
            case CURRENT_COUNTER_IS:// Only seen in Brian Howarth games so far
                if(CurrentCounter!=dv) {
                    return(0);
                }
                break;
        }
        cc++;
    }
    // Actions
    act[0]=Actions[ct].Action[0];
    act[2]=Actions[ct].Action[1];
    act[1]=act[0]%150;
    act[3]=act[2]%150;
    act[0]/=150;
    act[2]/=150;
    cc=0;
    pptr=0;
    while(cc<4) {
        if(act[cc]>=1 && act[cc]<52) {
            OutputMessage(act[cc]);
            Output("\n");
        } else if(act[cc]>101) {
            OutputMessage(act[cc]-50);
            Output("\n");
        } else switch(act[cc]) {
            case ACTION_NOP:
                break;
            case ACTION_GET_ITEM:
                if(CountCarried()==GameHeader.MaxCarry) {
                    if(Options&YOUARE) {
                        Output("You are carrying too much.\n");
                    } else {
                        Output("I've too much to carry!\n");
                    }
                    break;
                }
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
                Items[param[pptr++]].Location= CARRIED;
                break;
            case ACTION_DROP_ITEM:
                Redraw=1;
                Items[param[pptr++]].Location=MyLoc;
                break;
            case ACTION_MOVE_TO_ROOM:
                Redraw=1;
                MyLoc=param[pptr++];
                break;
            case ACTION_DESTROY_ITEM:
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
                Items[param[pptr++]].Location=0;
                break;
            case ACTION_SET_DARK:
                BitFlags|=1L<<DARKBIT;
                break;
            case ACTION_CLEAR_DARK:
                BitFlags&=~(1L<<DARKBIT);
                break;
            case ACTION_SET_FLAG:
                BitFlags|=(1L<<param[pptr++]);
                break;
            case ACTION_DESTROY_ITEM2:
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
                Items[param[pptr++]].Location=0;
                break;
            case ACTION_CLEAR_FLAG:
                BitFlags&=~(1L<<param[pptr++]);
                break;
            case ACTION_PLAYER_DEATH:
                if(Options&YOUARE) {
                    Output("You are dead.\n");
                } else {
                    Output("I am dead.\n");
                }
                BitFlags&=~(1L<<DARKBIT);
                MyLoc=GameHeader.NumRooms;
                break;
            case ACTION_ITEM_TO_ROOM: {
                uint8_t i=param[pptr++];
                Items[i].Location=param[pptr++];
                Redraw=1;
                break;
            }
            case ACTION_GAME_OVER:
                // Note: death/game over
                Output("The game is now over.\n");
                Restart=1; // just restart
                break;
            case ACTION_DESC_ROOM:
                Look(1);
                break;
            case ACTION_SCORE: {
                // Note: death/game over
                uint8_t ct=0;
                uint8_t n=0;
                while(ct<=GameHeader.NumItems) {
                    if(Items[ct].Location==GameHeader.TreasureRoom &&
                      *Items[ct].Text=='*') {
                        n++;
                    }
                    ct++;
                }
                if(Options&YOUARE) {
                    Output("You have stored ");
                } else {
                    Output("I've stored ");
                }
                OutputNumber(n);
                Output(" treasures.  On a scale of 0 to 100, that rates ");
                OutputNumber((n*100)/GameHeader.Treasures);
                Output(".\n");
                if(n==GameHeader.Treasures) {
                    Output("Well done.\nThe game is now over.\n");
                    Restart=1; // just restart
                }
                break;
            }
            case ACTION_INVENTORY: {
                uint8_t ct=0;
                uint8_t f=0;
                if(Options&YOUARE) {
                    Output("You are carrying:\n");
                } else {
                    Output("I'm carrying:\n");
                }
                while(ct<=GameHeader.NumItems) {
                    if(Items[ct].Location==CARRIED) {
                        if(f==1) {
                            Output(" - ");
                        }
                        f=1;
                        OutputPetscii(Items[ct].Text, 0); /* PETSCII, use OutBuf */
                    }
                    ct++;
                }
                if(f==0) {
                    Output("Nothing");
                }
                Output(".\n");
                break;
            }
            case ACTION_SET_FLAG_0:
                BitFlags|=(1L<<0);
                break;
            case ACTION_CLEAR_FLAG_0:
                BitFlags&=~(1L<<0);
                break;
            case ACTION_REFILL_LAMP:
                GameHeader.LightTime=LightRefill;
                if(Items[LIGHT_SOURCE].Location==MyLoc) {
                    Redraw=1;
                }
                Items[LIGHT_SOURCE].Location=CARRIED;
                BitFlags&=~(1L<<LIGHTOUTBIT);
                break;
            case ACTION_CLEAR_SCREEN:
                ClearScreen();
                gotoxy(0, TopHeight+2);
                break;
            case ACTION_SAVE_GAME:
                SaveGame();
                break;
            case ACTION_SWAP_ITEM: {
                uint8_t i1=param[pptr++];
                uint8_t i2=param[pptr++];
                uint8_t t=Items[i1].Location;
                if(t==MyLoc || Items[i2].Location==MyLoc) {
                    Redraw=1;
                }
                Items[i1].Location=Items[i2].Location;
                Items[i2].Location=t;
                break;
            }
            case ACTION_CONTINUE:
                continuation=1;
                break;
            case ACTION_TAKE_ITEM:
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
                Items[param[pptr++]].Location= CARRIED;
                break;
            case ACTION_PUT_ITEM_WITH: {
                uint8_t i1,i2;
                i1=param[pptr++];
                i2=param[pptr++];
                if(Items[i1].Location==MyLoc) {
                    Redraw=1;
                }
                Items[i1].Location=Items[i2].Location;
                if(Items[i2].Location==MyLoc) {
                    Redraw=1;
                }
                break;
            }
            case ACTION_LOOK:    // Looking at adventure ..
                Look(1);
                break;
            case ACTION_DEC_COUNTER:
                if(CurrentCounter>=0) {
                    CurrentCounter--;
                }
                break;
            case ACTION_PRINT_COUNTER:
                OutputNumber(CurrentCounter);
                break;
            case ACTION_SET_COUNTER:
                CurrentCounter=param[pptr++];
                break;
            case ACTION_SWAP_ROOM: {
                uint8_t t=MyLoc;
                MyLoc=SavedRoom;
                SavedRoom=t;
                Redraw=1;
                break;
            }
            case ACTION_SELECT_COUNTER: {
                /* This is somewhat guessed. Claymorgue always
                   seems to do select counter n, thing, select counter n,
                   but uses one value that always seems to exist. Trying
                   a few options I found this gave sane results on ageing */
                uint8_t t=param[pptr++];
                uint16_t c1=CurrentCounter;
                CurrentCounter=Counters[t];
                Counters[t]=c1;
                break;
            }
            case ACTION_ADD_COUNTER:
                CurrentCounter+=param[pptr++];
                break;
            case ACTION_SUB_COUNTER:
                CurrentCounter-=param[pptr++];
                if(CurrentCounter< -1) {
                    CurrentCounter= -1;
                }
                /* Note: This seems to be needed. I don't yet
                   know if there is a maximum value to limit too */
                break;
            case ACTION_PRINT_NOUN:
                Output(NounText);
                break;
            case ACTION_PRINT_NOUN_CR:
                Output(NounText);
                Output("\n");
                break;
            case ACTION_PRINT_CR:
                Output("\n");
                break;
            case ACTION_SWAP_ROOM_VALUE: {
                /* Changed this to swap location<->roomflag[x]
                   not roomflag 0 and x */
                uint8_t p=param[pptr++];
                uint8_t sr=MyLoc;
                MyLoc=RoomSaved[p];
                RoomSaved[p]=sr;
                Redraw=1;
                break;
            }
            case ACTION_PAUSE:
                sleep(2);    /* DOC's say 2 seconds. Spectrum times at 1.5 */
                break;
            case ACTION_SAGA_PIC:
                pptr++;
                /* SAGA draw picture n */
                /* Spectrum Seas of Blood - start combat ? */
                /* Poking this into older spectrum games causes a crash */
                break;
            default:
                // note: normally to error channel
                print("Unknown action ");
                print_number(act[cc]);
                print(" [Param begins ");
                print_number(param[pptr]);
                print_char(' ');
                print_number(param[pptr+1]);
                print("]\n");
                break;
        }
        cc++;
    }
    return(1+continuation);
}

int8_t PerformActions(int8_t vb,int8_t no) {
    static uint8_t disable_sysfunc=0;
    uint8_t d=BitFlags&(1L<<DARKBIT);
    uint16_t ct=0;
    int8_t fl;
    uint8_t doagain=0;
    if(vb==VERB_GO && no == -1 ) {
        Output("Give me a direction too.\n");
        return(0);
    }
    if(vb==VERB_GO && no>=1 && no<=6) {
        uint8_t nl;
        if(Items[LIGHT_SOURCE].Location==MyLoc ||
           Items[LIGHT_SOURCE].Location==CARRIED) {
               d=0;
        }
        if(d) {
            Output("Dangerous to move in the dark!\n");
        }
        nl=Rooms[MyLoc].Exits[no-1];
        if(nl!=0) {
            MyLoc=nl;
            Output("O.K.\n");
            return(0);
        }
        if(d) {
            // Note: death
            if(Options&YOUARE) {
                Output("You fell down and broke your neck.\n");
            } else {
                Output("I fell down and broke my neck.\n");
            }
            Restart=1;
            return(0);
        }
        if(Options&YOUARE) {
            Output("You can't go in that direction.\n");
        } else {
            Output("I can't go in that direction.\n");
        }
        return(0);
    }
    fl= -1;
    while(ct<=GameHeader.NumActions) {
        uint16_t vv,nv;
        vv=Actions[ct].Vocab;
        if(vb!=0 && (doagain&&vv!=0)) {
            break;
        }
        if(vb!=0 && !doagain && fl== 0) {
            break;
        }
        nv=vv%150;
        vv/=150;
        if((vv==vb)||(doagain&&Actions[ct].Vocab==0)) {
            if((vv==0 && RandomPercent(nv))||doagain||
                (vv!=0 && (nv==no||nv==0))) {
                uint8_t f2;
                if(fl== -1) {
                    fl= -2;
                }
                if((f2=PerformLine(ct))>0) {
                    fl=0;
                    if(f2==2) {
                        doagain=1;
                    }
                    if(vb!=0 && doagain==0) {
                        return(0);
                    }
                }
            }
        }
        ct++;
        if(Actions[ct].Vocab!=0) {
            doagain=0;
        }
    }
    if(fl!=0 && disable_sysfunc==0) {
        int8_t i;
        if(Items[LIGHT_SOURCE].Location==MyLoc ||
           Items[LIGHT_SOURCE].Location==CARRIED) {
               d=0;
        }
        if(vb==VERB_TAKE || vb==VERB_DROP) {
            if(vb==VERB_TAKE) {
                if(strcasecmp(NounText,"ALL")==0) {
                    uint8_t ct=0;
                    uint8_t f=0;
                    
                    if(d) {
                        Output("It is dark.\n");
                        return 0;
                    }
                    while(ct<=GameHeader.NumItems) {
                        if(Items[ct].Location==MyLoc && Items[ct].AutoGet!=NULL && Items[ct].AutoGet[0]!='*') {
                            no=WhichWord(Items[ct].AutoGet,Nouns);
                            disable_sysfunc=1;    /* Don't recurse into auto get ! */
                            PerformActions(vb,no);    /* Recursively check each items table code */
                            disable_sysfunc=0;
                            if(CountCarried()==GameHeader.MaxCarry) {
                                if(Options&YOUARE) {
                                    Output("You are carrying too much.\n");
                                } else {
                                    Output("I've too much to carry.\n");
                                }
                                return(0);
                            }
                            Items[ct].Location= CARRIED;
                            Redraw=1;
                            OutputPetscii(Items[ct].Text, 0); /* PETSCII, use OutBuf  */
                            Output(": O.K.\n");
                            f=1;
                        }
                        ct++;
                    }
                    if(f==0) {
                        Output("Nothing taken.\n");
                    }
                    return(0);
                }
                if(no==-1) {
                    Output("What ?\n");
                    return(0);
                }
                if(CountCarried()==GameHeader.MaxCarry) {
                    if(Options&YOUARE) {
                        Output("You are carrying too much.\n");
                    } else {
                        Output("I've too much to carry.\n");
                    }
                    return(0);
                }
                i=MatchUpItem(NounText,MyLoc);
                if(i==-1) {
                    if(Options&YOUARE) {
                        Output("It is beyond your power to do that.\n");
                    } else {
                        Output("It's beyond my power to do that.\n");
                    }
                    return(0);
                }
                Items[i].Location= CARRIED;
                Output("O.K.\n");
                Redraw=1;
                return(0);
            }
            if(vb==VERB_DROP) {
                if(strcasecmp(NounText,"ALL")==0) {
                    uint8_t ct=0;
                    uint8_t f=0;
                    while(ct<=GameHeader.NumItems) {
                        if(Items[ct].Location==CARRIED && Items[ct].AutoGet && Items[ct].AutoGet[0]!='*') {
                            no=WhichWord(Items[ct].AutoGet,Nouns);
                            disable_sysfunc=1;
                            PerformActions(vb,no);
                            disable_sysfunc=0;
                            Items[ct].Location=MyLoc;
                            OutputPetscii(Items[ct].Text, 0); //  PETSCII, use OutBuf
                            Output(": O.K.\n");
                            Redraw=1;
                            f=1;
                        }
                        ct++;
                    }
                    if(f==0) {
                        Output("Nothing dropped.\n");
                    }
                    return(0);
                }
                if(no==-1) {
                    Output("What ?\n");
                    return(0);
                }
                i=MatchUpItem(NounText,CARRIED);
                if(i==-1) {
                    if(Options&YOUARE) {
                        Output("It's beyond your power to do that.\n");
                    } else {
                        Output("It's beyond my power to do that.\n");
                    }
                    return(0);
                }
                Items[i].Location=MyLoc;
                Output("O.K.\n");
                Redraw=1;
                return(0);
            }
        }
    }
    return(fl);
}

void CheckLight() {
    // Brian Howarth games seem to use -1 for forever
    if(Items[LIGHT_SOURCE].Location/*==-1*/!=DESTROYED && GameHeader.LightTime!= -1) {
        GameHeader.LightTime--;
        if(GameHeader.LightTime<1) {
            BitFlags|=(1L<<LIGHTOUTBIT);
            if(Items[LIGHT_SOURCE].Location==CARRIED ||
                Items[LIGHT_SOURCE].Location==MyLoc) {
                if(Options&SCOTTLIGHT) {
                    Output("Light has run out! ");
                } else {
                    Output("Your light has run out. ");
                }
            }
            if(Options&PREHISTORIC_LAMP) {
                Items[LIGHT_SOURCE].Location=DESTROYED;
            }
        } else if(GameHeader.LightTime<25) {
            if(Items[LIGHT_SOURCE].Location==CARRIED ||
                Items[LIGHT_SOURCE].Location==MyLoc) {
        
                if(Options&SCOTTLIGHT) {
                    Output("Light runs out in ");
                    OutputNumber(GameHeader.LightTime);
                    Output(" turns. ");
                } else {
                    if(GameHeader.LightTime%5==0) {
                        Output("Your light is growing dim. ");
                    }
                }
            }
        }
    }
}

// Main Game Loop here
uint8_t GameLoop() {
    int8_t vb=0,no=0;

    while(1) {
        // Note: Redraws/Redraw checks removed, using look hack after prompt
        PerformActions(0,0);
        if(Restart!=0) {
            Restart=0;
            print("\nPress ");
            if (SavedGame!=NULL) {
                print("L to load \"");
                print(SavedGame);
                print("\"\nOR Press R to restart\n");
                return(cgetc()); // Returns to outer loop
            }
            print("any key to restart\n");
            cgetc();
            return('r');
        } else {
            GetInput(&vb,&no);
            switch(PerformActions(vb,no)) {
                case -1:
                    Output("I don't understand your command.\n");
                    break;
                case -2:
                    Output("I can't do that yet.\n");
                    break;
            }
            CheckLight();
        }
    }
}

// Parse arg to Options, or fail
// return sargument offset for remaining args: (dat/bdat, savegame)
uint8_t ParseArgs(uint8_t argc, uint8_t *argv[]) {
    uint8_t argo = 0;
    while(argv[1]) {
        if(*argv[1]!='-') {
            break;
        }
        switch(argv[1][1]) {
            case 'y':
                Options|=YOUARE;
                break;
            case 'i':
                Options&=~YOUARE;
                break;
            case 'd':
                Options|=DEBUGGING;
                break;
            case 's':
                Options|=SCOTTLIGHT;
                break;
            case 'p':
                Options|=PREHISTORIC_LAMP;
                break;
            case 'h':
            default:
                print(argv[0]);
                print(": [-h] [-y] [-s] [-i] [-d] [-p] <gamename> [savedgame].\n\r");
                print(" -h:help\n\r -y:'you are'\n\r -s:scottlight\n\r -i:'i am'\n\r -d:debug\n\r -p:old lamp behavior\n\r gamename:required\n\r savedgame:optional\n\r");
                exit(1);
        }
        if(argv[1][2]!=0) {
            // opt // fprintf(stderr,"%s: option -%c does not take a parameter.\n\r", argv[0],argv[1][1]);
            print(argv[0]);
            print(": option -");
            print_char(argv[1][1]);
            print("does not take a parameter.\n\r");
            exit(1);
        }
        argv++; // next arg
        argc--; // one less arg to process
        argo++; // count the options
    }

    if(argc!=2 && argc!=3) {
        print("run:rem <db file> <save file>\n\r");
        exit(1);
    }
    
    return(argo);
}

void GameSetup(uint8_t *filename, uint8_t *savedgame) {
    uint8_t textColor = COLOR_BLACK;
    uint8_t bgcolorColor = COLOR_GRAY1;
    TopHeight = 10;
#if defined(__C128__)
    if((PEEK(C128_MODE) & C128_MODE_80COL) == C128_MODE_80COL) {
        Width = 80;
        fast(); // enable 2mhz mode
    } else {
        Width = 40;
    }
    textColor = COLOR_BLACK;
    bgcolorColor = COLOR_GRAY3;
#elif defined(__C64__)
    Width = 40;
#endif

    OutReset();
    ClearScreen();
    textcolor (COLOR_LIGHTBLUE);     // Light blue
    bgcolor (COLOR_BLACK);        // Black
    bordercolor (COLOR_GRAY1);

    print("ScottFree{NAME} {VERSION}, by M. Seelye\
A c{NAME} port of:\
ScottFree, Scott Adams game driver in C\
Release 1.14b(PC), (c) 1993,1994,1995\
Swansea University Computer Society.\
Distributed under the GNU software\nlicense\n\n");

    print("MemFree(");
    print_number(_heapmemavail());
    print(")\nLoading ");
    print(filename);
    print_char('\n');

#if defined(__C128__)
    // Install custom em ram bank 1 driver
    em_install(&c128_ram_emd);
#endif

    LoadDatabase(filename,(Options&DEBUGGING)?1:0);  // load DAT/BDAT file

    // DEBUG
    //print("MemFree(");
    //print_number(_heapmemavail());
    //print(")\n");
    //cgetc();

    // Load Saved Game
    if(savedgame!=NULL) {
        LoadGame(savedgame);
    }

    // seed from c64/c128 $a2 and $d012
    srand((PEEK(CBM_TIME_LOW) << 8) | (PEEK(VIC.rasterline)));

    // Initial Colors
#if defined(__C128__)
    SetColors(textColor, bgcolorColor, bgcolorColor);
#else
    textcolor (textColor);
    bgcolor (bgcolorColor);
    bordercolor (bgcolorColor);
#endif
}

uint8_t main(uint8_t argc, uint8_t *argv[]) {
    int8_t argo = 0;
    argo = ParseArgs(argc, argv);

    GameSetup(argv[argo+1], (argc-argo==3)? argv[argo+2] : NULL);

    // Outer Loop
    // Loops here, restart flag used to reset/restart the game.
    while(1) {
        ClearScreen();
        gotoxy(0, TopHeight+1);
        Redraw = 1;
        NewGame(GameLoop()); // Loops until Restart flag set;
    }
    return(0);
}
