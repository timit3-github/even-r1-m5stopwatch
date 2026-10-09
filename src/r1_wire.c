/* SPDX-License-Identifier: BSL-1.0 AND MIT
 * Copyright (c) 2026 even-r1-esp32s3 contributors.
 * Copyright (c) 2026 openCFW contributors. See NOTICE.md and LICENSES. */
#include "r1_wire.h"
#include <string.h>
static uint16_t u16(const uint8_t *p) { return p[0] | (uint16_t)p[1]<<8; }
static uint32_t u32(const uint8_t *p) { return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static void w16(uint8_t *p, uint16_t x) { p[0]=x; p[1]=x>>8; }
uint32_t r1_crc32(const uint8_t *p,size_t n) {
 uint32_t c=0;
 while(n--) { c^=(uint32_t)*p++<<24; for(int i=0;i<8;i++) c=(c&0x80000000)?(c<<1)^0x1edc6f41:c<<1; }
 return c;
}
static uint16_t modbus_byte(uint16_t c,uint8_t b) {
 c^=b;for(int i=0;i<8;i++) c=(c&1)?(c>>1)^0xa001:c>>1;
 return c;
}
uint16_t r1_modbus(const uint8_t *p,size_t n) {
 uint16_t c=0xffff;
 while(n--) c=modbus_byte(c,*p++);
 return c;
}
uint16_t r1_model_modbus(const uint8_t *p,size_t n) {
 if(!p || n<12) return 0;
 uint16_t c=0xffff;
 for(size_t i=0;i<n;i++) c=modbus_byte(c,i==10 || i==11?0:p[i]);
 return c;
}
static uint16_t ccitt_byte(uint16_t c,uint8_t b) {
 c=(uint16_t)((c>>8)|(c<<8)); c^=b; c^=(c&255)>>4; c^=(uint16_t)(c<<12); c^=(uint16_t)((c&255)<<5); return c;
}
uint16_t r1_phone_checksum(const uint8_t *p,size_t n) {
 if(n<12) return 0;
 uint16_t c=0xffff;
 const uint8_t indices[]={0,1,2,3,5,6,7,8};
 for(size_t i=0;i<sizeof(indices);i++) c=ccitt_byte(c,p[indices[i]]);
 c=ccitt_byte(c,0);
 for(size_t i=12;i<n;i++) c=ccitt_byte(c,p[i]);
 return c;
}
int r1_receive(struct r1_rx *s,const uint8_t *p,size_t n) {
 if(n<5 || n>244 || p[0]>16) goto bad;
 uint32_t c=u32(p+1); int seq=p[0];
 if(!s->active) { s->used=0; s->crc=c; s->expected=seq; s->active=true; }
 if(seq!=s->expected || c!=s->crc || s->used+n-5>sizeof(s->data)) goto bad;
 memcpy(s->data+s->used,p+5,n-5); s->used+=n-5; s->expected=seq-1;
 if(seq) return 0;
 s->active=false;
 if(r1_crc32(s->data,s->used)!=s->crc) goto bad;
 return 1;
 bad: memset(s,0,sizeof(*s)); return -1;
}
bool r1_decode(const uint8_t *p,size_t n,struct r1_model *m) {
 if(!p || !m || n<12 || n>R1_MESSAGE_MAX || p[0]!=100 || p[2]<100 || u16(p+8)!=n) return false;
 uint16_t received=u16(p+10);
 bool compact=received==r1_phone_checksum(p,n),modbus=received==r1_model_modbus(p,n);
 if(!compact && !modbus) return false;
 enum r1_checksum_scheme scheme=compact && modbus?R1_CHECKSUM_AMBIGUOUS:
    modbus?R1_CHECKSUM_MODEL_MODBUS:R1_CHECKSUM_COMPACT_CCITT;
 *m=(struct r1_model){.module=p[1],.module_version=p[2],.serial=u16(p+3),.status=p[5],.command=p[6],.subcommand=p[7],.payload=p+12,.payload_len=n-12,.checksum_scheme=scheme};
 return true;
}
size_t r1_encode(const struct r1_model *m,uint8_t status,const uint8_t *payload,size_t n,uint8_t *out,size_t cap) {
 if(n>R1_MESSAGE_MAX-12 || cap<n+12 || (n && !payload)) return 0;
 memset(out,0,n+12); out[0]=100; out[1]=m->module; out[2]=100; w16(out+3,m->serial); out[5]=status; out[6]=m->command; out[7]=m->subcommand; w16(out+8,n+12);
 if(n) memcpy(out+12,payload,n);
 w16(out+10,r1_model_modbus(out,n+12)); return n+12;
}
size_t r1_fragment(const uint8_t *p,size_t n,size_t index,uint8_t *out,size_t cap) {
 if(n>4062 || index>n/239) return 0;
 size_t off=index*239, len=n-off; if(len>239) len=239;
 if(cap<len+5) return 0;
 out[0]=(uint8_t)(n/239-index); uint32_t c=r1_crc32(p,n);
 out[1]=c;out[2]=c>>8;out[3]=c>>16;out[4]=c>>24;
 if(len) memcpy(out+5,p+off,len);
 return len+5;
}
