#if defined(__C128__)

#include <c128.h>
#define EOL "\n"
#define C128_MODE 0xd7
#define C128_MODE_80COL 0x80

#elif defined(__C64__)

#define EOL "\n\r"

#endif

#define CBM_CURRENT_DEVICE_NUMBER 0xba
#define CBM_TIME_LOW 0xa2

#ifndef PEEK
    #define PEEK(addr) (*(unsigned char*) (addr))
#endif


// new option that autoloads saved game on restart
#define RESTORE_SAVED_ON_RESTART 32

#define VERB_GO       1
#define VERB_TAKE     10
#define VERB_DROP     18

#define ACTION_NOP              0
#define ACTION_GET_ITEM        52 /* Get item <arg>. Checks if you can carry it first */
#define ACTION_DROP_ITEM       53 /* Drops item <arg> */
#define ACTION_MOVE_TO_ROOM    54 /* Moves to room <arg> */
#define ACTION_DESTROY_ITEM    55 /* Item <arg> is removed from the game (put in room 0) */
#define ACTION_SET_DARK        56 /* The darkness flag is set */
#define ACTION_CLEAR_DARK      57 /* The darkness flag is cleared */
#define ACTION_SET_FLAG        58 /* Bitflag <arg> is set */
#define ACTION_DESTROY_ITEM2   59 /* The same as 55 (it seems - I'm cautious about this) */
#define ACTION_CLEAR_FLAG      60 /* BitFlag <arg> is cleared */
#define ACTION_PLAYER_DEATH    61 /* Death. Dark flag cleared, player moved to last room */
#define ACTION_ITEM_TO_ROOM    62 /* Item <arg> put in room <arg> */
#define ACTION_GAME_OVER       63 /* Game over. */
#define ACTION_DESC_ROOM       64 /* Describe room */
#define ACTION_SCORE           65 /* Score */
#define ACTION_INVENTORY       66 /* Inventory */
#define ACTION_SET_FLAG_0      67 /* BitFlag 0 is set	*/
#define ACTION_CLEAR_FLAG_0    68 /* BitFlag 0 is cleared */
#define ACTION_REFILL_LAMP     69 /* Refill lamp (?) */
#define ACTION_CLEAR_SCREEN    70 /* Screen is cleared. This varies by driver from no effect upwards */
#define ACTION_SAVE_GAME       71 /* Saves the game. Choices of filename etc depend on the driver alone. */
#define ACTION_SWAP_ITEM       72 /* Swap item <arg1> and item <arg2> rooms */
#define ACTION_CONTINUE        73 /* Continue with next line (the next line starts verb 0 noun 0) */
#define ACTION_TAKE_ITEM       74 /* Take item <arg> - no check is done too see if it can be carried. */
#define ACTION_PUT_ITEM_WITH   75 /* Put item <arg1> with item <arg2> - Not certain seems to do this from examination of Claymorgue */
#define ACTION_LOOK            76 /* Look (same as 64 ?? - check) */
#define ACTION_DEC_COUNTER     77 /* Decrement current counter. Will not go below 0 */
#define ACTION_PRINT_COUNTER   78 /* Print current counter value. Some drivers only cope with 0-99 apparently */
#define ACTION_SET_COUNTER     79 /* Set current counter value */
#define ACTION_SWAP_ROOM       80 /* ? Swap room with current room-swap flag */
#define ACTION_SELECT_COUNTER  81 /* Select a counter. Current counter is swapped with backup counter <arg> */
#define ACTION_ADD_COUNTER     82 /* Add <arg> to current counter */
#define ACTION_SUB_COUNTER     83 /* Subtract <arg> from current counter */
#define ACTION_PRINT_NOUN      84 /* Echo noun player typed without CR */
#define ACTION_PRINT_NOUN_CR   85 /* Echo the noun the player typed with CR */
#define ACTION_PRINT_CR        86 /* CR */
#define ACTION_SWAP_ROOM_VALUE 87 /* ? Swap current room with backup room-swap value <arg> */
#define ACTION_PAUSE           88 /* Wait 2 seconds */
#define ACTION_SAGA_PIC        89 /* Not Implemented: SAGA Pic */

#define ADD_ARG_TO_PARAMS                           0
#define ITEM_CARRIED                                1
#define ITEM_IN_ROOM_WITH_PLAYER                    2
#define ITEM_CARRIED_OR_IN_ROOM_WITH_PLAYER         3
#define PLAYER_IN_ROOM                              4
#define ITEM_NOT_IN_ROOM_WITH_PLAYER                5
#define ITEM_NOT_CARRIED                            6
#define PLAYER_NOT_IN_ROOM                          7
#define BIT_FLAG_IS_SET                             8
#define BIT_FLAG_IS_CLEARED                         9
#define SOMETHING_CARRIED                          10
#define NOTHING_CARRIED                            11
#define ITEM_NOT_CARRIED_NOR_IN_ROOM_WITH_PLAYER   12
#define ITEM_IS_IN_GAME                            13
#define ITEM_IS_NOT_IN_GAME                        14
#define CURRENT_COUNTER_LESS_THAN_OR_EQUAL_TO      15
#define CURRENT_COUNTER_GREATER_THAN               16
#define ITEM_IN_INITIAL_ROOM                       17
#define ITEM_NOT_IN_INITIAL_ROOM                   18
#define CURRENT_COUNTER_IS                         19 // Brian Howarth Games

