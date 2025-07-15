
/* istring.c */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>




uint16_t macdefSP;
uint16_t maxMacDef;
uint16_t topStr;



int newString ( uint16_t len , const uint8_t *str );
bool strequ(int strId, uint8_t const *str, int len);
uint8_t *idToStr( int loc );
bool defMacro ( int idLen , uint8_t *id , int valLen , uint8_t *val );
bool useMacro ( int len , char *str );
void dropMacro ( int newTop );
int macGetc ( void );

void error(char const *fmt, ...);
void fatal(char const *fmt, ...);
