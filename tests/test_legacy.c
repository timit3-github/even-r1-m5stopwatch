#include "r1_legacy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Request fixtures are the old G2 sender's constant frames. Response fixtures
 * are derived from the stock R1 handlers, independently of this parser. */
int main(void) {
    struct r1_legacy_plan plan;
    const uint8_t pair[] = {0,0x35,0x88,0};
    assert(r1_legacy_parse(pair,sizeof(pair),&plan));
    assert(plan.action==R1_LEGACY_PAIR_GLASSES && plan.response_len==0);
    const uint8_t enable[] = {0,0x1a,0x85,1,0,0xaa,0xaa,0xaa};
    const uint8_t enable_reply[] = {0,0x1a,0x85,1,1,0xaa,0xaa};
    assert(r1_legacy_parse(enable,sizeof(enable),&plan));
    assert(plan.action==R1_LEGACY_TOUCH_ENABLE && plan.enabled);
    assert(plan.response_len==sizeof(enable_reply) && !memcmp(plan.response,enable_reply,sizeof(enable_reply)));
    uint8_t disable[8];memcpy(disable,enable,8);disable[4]=0xff;
    assert(r1_legacy_parse(disable,8,&plan) && !plan.enabled);
    const uint8_t hold[] = {0,0x1a,0x8a,1,0x34,0x12};
    const uint8_t hold_reply[] = {0,0x1a,0x8a,1,1,0x12,0};
    assert(r1_legacy_parse(hold,sizeof(hold),&plan));
    assert(plan.hold_time_be==0x3412); /* preserve observed endian mismatch */
    assert(plan.response_len==sizeof(hold_reply) && !memcmp(plan.response,hold_reply,sizeof(hold_reply)));
    const uint8_t status[] = {0,0x1a,0x89,1,0xc0,0,0,0};
    assert(r1_legacy_parse(status,sizeof(status),&plan) && plan.glasses_status==0xc0);
    const uint8_t status_reply[] = {0,0x1a,0x89,1,1,0,0};
    assert(plan.response_len==sizeof(status_reply) && !memcmp(plan.response,status_reply,sizeof(status_reply)));
    const uint8_t heartbeat[] = {0,0x1a,0x94,1};
    const uint8_t heartbeat_reply[] = {0,0x1a,0x94,1,1};
    assert(r1_legacy_parse(heartbeat,sizeof(heartbeat),&plan));
    assert(plan.response_len==sizeof(heartbeat_reply) && !memcmp(plan.response,heartbeat_reply,sizeof(heartbeat_reply)));

    const uint8_t *known[]={pair,enable,hold,status,heartbeat};
    const size_t sizes[]={sizeof(pair),sizeof(enable),sizeof(hold),sizeof(status),sizeof(heartbeat)};
    for(size_t i=0;i<5;i++) {
        for(size_t n=0;n<sizes[i];n++) assert(!r1_legacy_parse(known[i],n,&plan));
        uint8_t bad[37]={0};memcpy(bad,known[i],sizes[i]);bad[3]^=1;
        assert(!r1_legacy_parse(bad,sizes[i],&plan));
        memcpy(bad,known[i],sizes[i]);assert(!r1_legacy_parse(bad,sizes[i]+1,&plan));
    }
    assert(!r1_legacy_parse(NULL,4,&plan));
    assert(!r1_legacy_parse(pair,4,NULL));
    uint8_t unknown[]={0,0x1a,0xfe,1};
    assert(r1_legacy_parse(unknown,4,&plan) && plan.action==R1_LEGACY_UNSUPPORTED && !plan.response_len);
    uint8_t out[11];
    const uint8_t touch[]={0,9,0x61,0,1,0,0,0x78,0x56,0x34,0x12};
    assert(r1_legacy_touch(1,0,0,0x12345678,out,sizeof(out))==sizeof(touch));
    assert(!memcmp(out,touch,sizeof(touch)));
    const uint8_t types[]={0,1,2,4,5,8};
    for(size_t i=0;i<sizeof(types);i++) assert(r1_legacy_touch(types[i],7,9,123,out,11)==11);
    assert(!r1_legacy_touch(3,0,0,0,out,11));
    assert(!r1_legacy_touch(1,0,0,0,out,10));
    assert(!r1_legacy_touch(1,0,0,0,NULL,11));

    uint8_t targets[12]={0}, peer[6]={1,2,3,4,5,6};
    assert(!r1_target_valid(targets) && r1_target_match(targets,peer)==-1);
    memset(targets,0xff,12);assert(!r1_target_valid(targets) && r1_target_match(targets,peer)==-1);
    memcpy(targets+6,peer,6);assert(r1_target_match(targets,peer)==1);
    memcpy(targets,peer,6);assert(r1_target_match(targets,peer)==0);
    peer[0]=9;assert(r1_target_match(targets,peer)==-2);
    assert(r1_target_match(NULL,peer)==-2 && r1_target_match(targets,NULL)==-2);
    puts("legacy tests passed: fixed G2/R1 fixtures, no synthetic auth, touch, malformed lengths, targets");
}
