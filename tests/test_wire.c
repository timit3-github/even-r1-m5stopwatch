#include "r1_wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Independent CRC check values for the specified non-reflected CRC-32C. */
static uint32_t independent_crc(const uint8_t *p,size_t n) {
 uint32_t c=0;for(size_t i=0;i<n;i++) {for(int k=7;k>=0;k--) {int top=(c>>31)^((p[i]>>k)&1);c<<=1;if(top)c^=0x1edc6f41;}}return c;
}
static const uint8_t pair_request[]={100,1,100,0,0x3f,0,0,8,13,0,0,0,1};
int main(void) {
 assert(r1_modbus((const uint8_t *)"123456789",9)==0x4b37);
 assert(r1_crc32((const uint8_t *)"123456789",9)==independent_crc((const uint8_t *)"123456789",9));
 uint8_t req[sizeof(pair_request)];memcpy(req,pair_request,sizeof(req));uint16_t sum=r1_phone_checksum(req,sizeof(req));req[10]=sum;req[11]=sum>>8;
 /* Fixed vector from openCFW r1_ble_probe_frames.c at 832137ec. */
 const uint8_t oracle[]={0x00,0x1d,0x47,0xca,0x7b,0x64,0x01,0x64,0x00,0x3f,0x00,0x00,0x08,0x0d,0x00,0x5e,0xb9,0x01};
 uint8_t generated[244];size_t flen=r1_fragment(req,sizeof(req),0,generated,sizeof(generated));
 assert(flen==sizeof(oracle) && !memcmp(generated,oracle,sizeof(oracle)));
 struct r1_model m;assert(r1_decode(req,sizeof(req),&m));assert(m.serial==0x3f00 && m.subcommand==8 && m.payload_len==1 && m.payload[0]==1);
 uint8_t reply[64];const uint8_t ok=0;size_t n=r1_encode(&m,3,&ok,1,reply,sizeof(reply));assert(n==13 && reply[5]==3 && reply[3]==0 && reply[4]==0x3f);
 uint16_t reply_sum=reply[10]|reply[11]<<8;reply[10]=reply[11]=0;assert(reply_sum==r1_modbus(reply,n));
 uint8_t data[R1_MESSAGE_MAX],f[244];for(size_t i=0;i<sizeof(data);i++)data[i]=(uint8_t)(i*17+3);
 const size_t lens[]={0,12,13,238,239,240,478,4062};
 for(size_t k=0;k<sizeof(lens)/sizeof(lens[0]);k++) {
  size_t len=lens[k];struct r1_rx s={0};
  for(size_t i=0;i<=len/239;i++) {size_t fl=r1_fragment(data,len,i,f,sizeof(f));assert(fl>=5 && fl<=244);int rc=r1_receive(&s,f,fl);assert(rc==(i==len/239?1:0));}
  assert(s.used==len && !memcmp(s.data,data,len));
 }
 assert(r1_fragment(data,4063,0,f,sizeof(f))==0);
 struct r1_rx s={0};n=r1_fragment(data,240,0,f,sizeof(f));assert(r1_receive(&s,f,n)==0);n=r1_fragment(data,240,1,f,sizeof(f));f[1]^=1;assert(r1_receive(&s,f,n)==-1 && !s.active);
 n=r1_fragment(data,13,0,f,sizeof(f));f[5]^=1;assert(r1_receive(&s,f,n)==-1);
 assert(r1_receive(&s,f,4)==-1);f[0]=17;assert(r1_receive(&s,f,5)==-1);
 req[12]^=1;assert(!r1_decode(req,sizeof(req),&m));
 puts("wire tests passed: CRC, model, fragmentation, corrupt CRC, sequence bounds");
}
