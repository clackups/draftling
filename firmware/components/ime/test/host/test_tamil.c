/*
 * Host test of the Tamil input methods (Tamil 99, Typewriter,
 * Taravinyas): keys are typed into a fake document and the text is
 * compared with the expected Tamil (as UTF-8 escapes). Run ./run.sh.
 */
#include <stdio.h>
#include <string.h>
#include "ime.h"
static char doc[4096]; static size_t cur;
static size_t peek(char *b, size_t max){ size_t s=cur; size_t st=s; while(st>0 && doc[st-1]!='\n' && s-st<max) st--; if(s-st>max) st=s-max; memcpy(b,doc+st,s-st); return s-st;}
static bool rep(size_t del,const char*t){ size_t l=strlen(t); memmove(doc+cur-del+l, doc+cur, strlen(doc+cur)+1); memcpy(doc+cur-del,t,l); cur=cur-del+l; return true;}
static const ime_host_t H={peek,rep};
/* Type the keys into an empty document (Space as HID 0x2C). */
static void type(kb_ime_t k, const char *keys){
  doc[0]=0; cur=0; ime_commit();
  for(const char*p=keys;*p;p++){ char t[2]={*p,0};
    uint8_t kc = *p==' '?0x2C:0x04;
    if(!ime_handle_key(k,kc,0,t)){ rep(0,t); ime_commit(); } }
}
static int fails;
static void check(kb_ime_t k,const char*keys,const char*want){ type(k,keys); if(strcmp(doc,want)){fails++; printf("FAIL %s -> [%s] want [%s]\n",keys,doc,want);} else printf("ok   %-12s %s\n",keys,doc);}
int main(void){ ime_init(&H,NULL,NULL);
 kb_ime_t T=KB_IME_TAMIL99;
 check(T,"hq","\xE0\xAE\x95""\xE0\xAE\xBE"); check(T,"hf","\xE0\xAE\x95""\xE0\xAF\x8D"); check(T,"hh","\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\x95"); check(T,"hhh","\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\x95""\xE0\xAE\x95"); check(T,"hhhh","\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\x95""\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\x95");
 check(T,"ha","\xE0\xAE\x95"); check(T,"has","\xE0\xAE\x95""\xE0\xAE\x87"); check(T,"hah","\xE0\xAE\x95""\xE0\xAE\x95"); check(T,"hahh","\xE0\xAE\x95""\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\x95");
 check(T,"bh","\xE0\xAE\x99""\xE0\xAF\x8D""\xE0\xAE\x95"); check(T,";l","\xE0\xAE\xA8""\xE0\xAF\x8D""\xE0\xAE\xA4"); check(T,";ll","\xE0\xAE\xA8""\xE0\xAF\x8D""\xE0\xAE\xA4""\xE0\xAE\xA4"); check(T,";lll","\xE0\xAE\xA8""\xE0\xAF\x8D""\xE0\xAE\xA4""\xE0\xAE\xA4""\xE0\xAF\x8D""\xE0\xAE\xA4"); check(T,";al","\xE0\xAE\xA8""\xE0\xAE\xA4");
 check(T,"qs","\xE0\xAE\x86""\xE0\xAE\x87"); check(T,"jqs","\xE0\xAE\xAA""\xE0\xAE\xBE""\xE0\xAE\x87"); check(T,"lks/f","\xE0\xAE\xA4""\xE0\xAE\xAE""\xE0\xAE\xBF""\xE0\xAE\xB4""\xE0\xAF\x8D"); check(T,"Tq","\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\xB7""\xE0\xAE\xBE"); check(T,"Y","\xE0\xAE\xB8""\xE0\xAF\x8D""\xE0\xAE\xB0""\xE0\xAF\x80"); check(T,"hc","\xE0\xAE\x95""\xE0\xAF\x8A"); check(T,"kzd","\xE0\xAE\xAE""\xE0\xAF\x8C""\xE0\xAE\x89");
 check(T,"Ah","\xE0\xAF\xB9""\xE0\xAE\x95");
 kb_ime_t W=KB_IME_TAMIL_TW;
 check(W,"bf","\xE0\xAE\x95""\xE0\xAF\x86"); check(W,"bfh","\xE0\xAE\x95""\xE0\xAF\x8A"); check(W,"nfh","\xE0\xAE\x95""\xE0\xAF\x8B"); check(W,"if","\xE0\xAE\x95""\xE0\xAF\x88"); check(W,"bfs","\xE0\xAE\x95""\xE0\xAF\x8C"); check(W,"bfs;","\xE0\xAE\x95""\xE0\xAF\x86""\xE0\xAE\xB3""\xE0\xAF\x8D");
 check(W,"xs","\xE0\xAE\x94"); check(W,"Fh","\xE0\xAE\x95""\xE0\xAF\x81""\xE0\xAE\xBE"); check(W,"fh","\xE0\xAE\x95""\xE0\xAE\xBE"); check(W,"jp","\xE0\xAE\xA4""\xE0\xAE\xBF"); check(W,"o","\xE0\xAE\x9F""\xE0\xAE\xBF"); check(W,"E}","\xE0\xAE\xA8""\xE0\xAF\x82"); check(W,"L:","\xE0\xAE\x9F""\xE0\xAF\x82"); check(W,"g[","\xE0\xAE\xAA""\xE0\xAF\x81");
 check(W,"jkpH;","\xE0\xAE\xA4""\xE0\xAE\xAE""\xE0\xAE\xBF""\xE0\xAE\xB4""\xE0\xAF\x8D"); check(W,"b\\","\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\xB7""\xE0\xAF\x86"); check(W,"z]","\xE0\xAE\xA3""\xE0\xAF\x88"); check(W,"mk","\xE0\xAE\x85""\xE0\xAE\xAE");
 kb_ime_t V=KB_IME_TARAVINYAS;
 check(V,"fj","\xE0\xAE\x95"); check(V,"sl","\xE0\xAE\xB4"); check(V,"su","\xE0\xAE\xB3"); check(V,"wl","\xE0\xAE\xB0"); check(V,"wu","\xE0\xAE\xB2"); check(V,"dl","\xE0\xAE\xAF"); check(V,"du","\xE0\xAE\xAF"); check(V,"wi","\xE0\xAE\xB8"); check(V,"fi","");
 check(V,"rjvsjx","\xE0\xAE\xA4""\xE0\xAF\x81"); check(V,"vfjvfkx","\xE0\xAE\x95""\xE0\xAE\xBE"); check(V,"vfjvfjx","\xE0\xAE\x95"); check(V,"rlc","\xE0\xAE\x90"); check(V,"elx","\xE0\xAF\x8C"); check(V,"fkc","\xE0\xAE\x86"); check(V,"aj","\xE0\xAF\x8D"); check(V,"vak","\xE0\xAE\xB9"); check(V,"al","\xE0\xAE\x95""\xE0\xAF\x8D""\xE0\xAE\xB7");
 check(V,"rjvdjxvejvaj","\xE0\xAE\xA4""\xE0\xAE\xBF""\xE0\xAE\xAA""\xE0\xAF\x8D"); check(V,"vfl",""); check(V,"rjekvdjxvslvaj","\xE0\xAE\xA4\xE0\xAE\xAE\xE0\xAE\xBF\xE0\xAE\xB4\xE0\xAF\x8D");
 printf("%d failures\n",fails); return fails!=0; }
