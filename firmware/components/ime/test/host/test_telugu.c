/*
 * Host test of the Telugu input methods (RTS, Pothana, Apple): keys are
 * typed into a fake document and the text is compared with the
 * expected Telugu (as UTF-8 escapes). In the key strings, ' ' is Space
 * and '\\b' Backspace. Run ./run.sh.
 */
#include <stdio.h>
#include <string.h>
#include "ime.h"
static char doc[4096]; static size_t cur;
static size_t peek(char *b, size_t max){ size_t s=cur; size_t st=s; while(st>0 && doc[st-1]!='\n' && s-st<max) st--; if(s-st>max) st=s-max; memcpy(b,doc+st,s-st); return s-st;}
static bool rep(size_t del,const char*t){ size_t l=strlen(t); memmove(doc+cur-del+l, doc+cur, strlen(doc+cur)+1); memcpy(doc+cur-del,t,l); cur=cur-del+l; return true;}
static const ime_host_t H={peek,rep};
static void bs(void){ if(!cur) return; size_t n=1; while(n<cur && ((unsigned char)doc[cur-n]&0xC0)==0x80) n++; memmove(doc+cur-n,doc+cur,strlen(doc+cur)+1); cur-=n; }
/* Type the keys into an empty document, as the editor does: a key the
 * input method does not take is typed as it is. */
static void type(kb_ime_t k, const char *keys){
  doc[0]=0; cur=0; ime_commit();
  for(const char*p=keys;*p;p++){
    if(*p=='\b'){ if(!ime_handle_key(k,0x2A,0,NULL)) bs(); continue; }
    char t[2]={*p,0};
    uint8_t kc = *p==' '?0x2C:0x04;
    if(!ime_handle_key(k,kc,0,t)){ rep(0,t); ime_commit(); } }
  ime_commit();
}
static int fails;
static void check(kb_ime_t k,const char*name,const char*keys,const char*want){ type(k,keys); if(strcmp(doc,want)){fails++; printf("FAIL %s [%s] -> [%s] want [%s]\n",name,keys,doc,want);} else printf("ok   %-8s %s\n",name,keys);}
int main(void){ ime_init(&H,NULL,NULL);
 check(KB_IME_TELUGU_RTS,"rts","telugu ","\xE0\xB0\xA4\xE0\xB1\x86\xE0\xB0\xB2\xE0\xB1\x81\xE0\xB0\x97\xE0\xB1\x81 ");
 check(KB_IME_TELUGU_RTS,"rts","namaskaaram ","\xE0\xB0\xA8\xE0\xB0\xAE\xE0\xB0\xB8\xE0\xB1\x8D\xE0\xB0\x95\xE0\xB0\xBE\xE0\xB0\xB0\xE0\xB0\x82 ");
 check(KB_IME_TELUGU_RTS,"rts","kRshNa","\xE0\xB0\x95\xE0\xB1\x83\xE0\xB0\xB7\xE0\xB1\x8D\xE0\xB0\xA3");
 check(KB_IME_TELUGU_RTS,"rts","andhra","\xE0\xB0\x85\xE0\xB0\x82\xE0\xB0\xA7\xE0\xB1\x8D\xE0\xB0\xB0");
 check(KB_IME_TELUGU_RTS,"rts","amma","\xE0\xB0\x85\xE0\xB0\xAE\xE0\xB1\x8D\xE0\xB0\xAE");
 check(KB_IME_TELUGU_RTS,"rts","jnaanam.","\xE0\xB0\x9C\xE0\xB1\x8D\xE0\xB0\x9E\xE0\xB0\xBE\xE0\xB0\xA8\xE0\xB0\x82.");
 check(KB_IME_TELUGU_RTS,"rts","ksha","\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7");
 check(KB_IME_TELUGU_RTS,"rts","kha","\xE0\xB0\x96");
 check(KB_IME_TELUGU_RTS,"rts","12 ","\xE0\xB1\xA7\xE0\xB1\xA8 ");
 check(KB_IME_TELUGU_RTS,"rts","sundaram ","\xE0\xB0\xB8\xE0\xB1\x81\xE0\xB0\x82\xE0\xB0\xA6\xE0\xB0\xB0\xE0\xB0\x82 ");
 check(KB_IME_TELUGU_RTS,"rts","hindee","\xE0\xB0\xB9\xE0\xB0\xBF\xE0\xB0\x82\xE0\xB0\xA6\xE0\xB1\x80");
 check(KB_IME_TELUGU_RTS,"rts","n&ka","\xE0\xB0\xA8\xE0\xB1\x8D\xE0\xB0\x95");
 check(KB_IME_TELUGU_RTS,"rts","raamu","\xE0\xB0\xB0\xE0\xB0\xBE\xE0\xB0\xAE\xE0\xB1\x81");
 check(KB_IME_TELUGU_RTS,"rts","aemi","\xE0\xB0\x8F\xE0\xB0\xAE\xE0\xB0\xBF");
 check(KB_IME_TELUGU_RTS,"rts","rAm\bmu","\xE0\xB0\xB0\xE0\xB0\xBE\xE0\xB0\xAE\xE0\xB1\x81");
 check(KB_IME_TELUGU_RTS,"rts","x","\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7\xE0\xB1\x8D");
 check(KB_IME_TELUGU_RTS,"rts","prapaMcaM","\xE0\xB0\xAA\xE0\xB1\x8D\xE0\xB0\xB0\xE0\xB0\xAA\xE0\xB0\x82\xE0\xB0\x9A\xE0\xB0\x82");
 check(KB_IME_TELUGU_RTS,"rts","kaa|","\xE0\xB0\x95\xE0\xB0\xBE\xE0\xA5\xA4");
 check(KB_IME_TELUGU_RTS,"rts","am ","\xE0\xB0\x85\xE0\xB0\x82 ");
 check(KB_IME_TELUGU_POTHANA,"pothana","welugu","\xE0\xB0\xA4\xE0\xB1\x86\xE0\xB0\xB2\xE0\xB1\x81\xE0\xB0\x97\xE0\xB1\x81");
 check(KB_IME_TELUGU_POTHANA,"pothana","kA","\xE0\xB0\x95\xE0\xB0\xBE");
 check(KB_IME_TELUGU_POTHANA,"pothana","kai","\xE0\xB0\x95\xE0\xB0\x87");
 check(KB_IME_TELUGU_POTHANA,"pothana","khk","\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\x95");
 check(KB_IME_TELUGU_POTHANA,"pothana","a","\xE0\xB0\x85");
 check(KB_IME_TELUGU_POTHANA,"pothana","kf","\xE0\xB0\x95\xE0\xB0\x82");
 check(KB_IME_TELUGU_POTHANA,"pothana","XO","\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\xB7\xE0\xB1\x8B");
 check(KB_IME_TELUGU_APPLE,"apple","suaixi","\xE0\xB0\xA4\xE0\xB1\x86\xE0\xB0\xB2\xE0\xB1\x81\xE0\xB0\x97\xE0\xB1\x81");
 check(KB_IME_TELUGU_APPLE,"apple","johj","\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\x95\xE0\xB1\x87");
 check(KB_IME_TELUGU_APPLE,"apple","jhjo","\xE0\xB0\x95\xE0\xB1\x8D\xE0\xB0\x95\xE0\xB1\x87");
 check(KB_IME_TELUGU_APPLE,"apple","e","\xE0\xB0\x86");
 check(KB_IME_TELUGU_APPLE,"apple","je","\xE0\xB0\x95\xE0\xB0\xBE");
 check(KB_IME_TELUGU_APPLE,"apple","jqe","\xE0\xB0\x95\xE0\xB0\x86");
 check(KB_IME_TELUGU_APPLE,"apple","lbbe","\xE0\xB0\xA8\xE0\xB0\xAE\xE0\xB0\xAE\xE0\xB0\xBE");
 check(KB_IME_TELUGU_APPLE,"apple","rjo","\xE0\xB0\x87\xE0\xB0\x95\xE0\xB1\x87");
 /* AltGr keys pass through (the layout's own characters). */
 doc[0]=0; cur=0; ime_commit();
 if(ime_handle_key(KB_IME_TELUGU_POTHANA,0x2F,0x40,"[")){ fails++; printf("FAIL pothana AltGr + [ taken\n"); } else printf("ok   pothana AltGr\n");
 /* Shift + Space: zero-width non-joiner. */
 doc[0]=0; cur=0; ime_commit(); ime_handle_key(KB_IME_TELUGU_RTS,0x04,0,"k"); ime_handle_key(KB_IME_TELUGU_RTS,0x2C,0x02," "); ime_commit();
 if(strcmp(doc,"\xE0\xB0\x95\xE0\xB1\x8D\xE2\x80\x8C")){ fails++; printf("FAIL rts shift+space [%s]\n",doc);} else printf("ok   rts shift+space\n");
 printf("%d failures\n",fails); return fails!=0; }
