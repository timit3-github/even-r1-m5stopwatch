#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_store.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/util/util.h"
void ble_store_config_init(void);
#include "r1_config.h"
#include "r1_wire.h"
#include "r1_legacy.h"
static const char *TAG="R1TEST";
#define UUID(n) BLE_UUID128_INIT(0x1f,0x9d,0x32,0xf7,0xf1,0x3a,0x65,0x8e,0x03,0x45,0x05,0x4f,n,0x00,0xe8,0xba)
static const ble_uuid128_t service_uuid=UUID(1), rx1_uuid=UUID(0x10), tx1_uuid=UUID(0x11),rx2_uuid=UUID(0x12),tx2_uuid=UUID(0x13);
static uint16_t tx1_handle,tx2_handle;
static uint8_t own_addr_type;
static char device_name[24];
static uint8_t identity_addr[6];
static uint16_t notification_serial;
static nvs_handle_t settings;
static QueueHandle_t input_queue;
static struct ble_npl_callout ticker;
static uint8_t targets[12];
static bool source_enabled=true;
static uint32_t last_touch_tick;
static int selected_conn=-1;
struct console_command { char text[540]; };
struct link {
 bool used,notify1,notify2,phone,glasses,encrypted,phone_auth_pending;
 bool touch_enabled,initial_state_pending;
 uint8_t peer[6],peer_type,glasses_status;
 uint16_t handle; uint32_t generation;
 int64_t last_rx_ms,auth_due_ms;
 struct r1_rx rx;
};
static struct link links[3];
static uint32_t next_generation;
struct tx_packet { uint16_t conn,attr; uint32_t generation; uint8_t len,retries; int64_t queued_ms; uint8_t bytes[244]; };
static struct tx_packet tx_queue[32];
static unsigned tx_head,tx_count;
static void advertise(void);
static int gap_event(struct ble_gap_event *,void *);
static struct link *find_link(uint16_t h) {
 for(unsigned i=0;i<3;i++) if(links[i].used && links[i].handle==h) return &links[i];
 return NULL;
}
static int64_t now_ms(void) { return esp_timer_get_time()/1000; }
static bool assign_role(struct link *l,bool phone) {
 if((phone && l->glasses) || (!phone && l->phone)) {
  ESP_LOGW(TAG,"ROLE_CONFLICT conn=%u",l->handle);return false;
 }
 for(unsigned i=0;i<3;i++) if(links[i].used && &links[i]!=l &&
   (phone?links[i].phone:links[i].glasses)) {
  ESP_LOGW(TAG,"ROLE_OCCUPIED conn=%u owner=%u role=%s",l->handle,links[i].handle,phone?"phone":"glasses");return false;
 }
 if(phone) l->phone=true;else l->glasses=true;
 ESP_LOGI(TAG,"ROLE conn=%u role=%s",l->handle,phone?"phone":"glasses");return true;
}
static void log_bytes(const char *label,uint16_t conn,const uint8_t *p,size_t n) {
 ESP_LOGI(TAG,"%s conn=%u len=%u",label,conn,(unsigned)n);
 ESP_LOG_BUFFER_HEX_LEVEL(TAG,p,n,ESP_LOG_INFO);
}
/* All link/protocol/TX state is owned by the NimBLE host thread. */
static bool queue_packet(struct link *l,uint16_t attr,const uint8_t *p,size_t n) {
 if(!n || n>244 || tx_count==32) { ESP_LOGW(TAG,"TX_QUEUE_FULL_OR_OVERSIZE"); return false; }
 struct tx_packet *t=&tx_queue[(tx_head+tx_count)%32];
 *t=(struct tx_packet){.conn=l->handle,.attr=attr,.generation=l->generation,.len=n,.queued_ms=now_ms()};
 memcpy(t->bytes,p,n);tx_count++;return true;
}
static void reply(struct link *l,const struct r1_model *req,uint8_t status,const uint8_t *p,size_t n) {
 if(!l->notify2) { ESP_LOGW(TAG,"NO_EUS_SUBSCRIPTION conn=%u",l->handle);return; }
 uint8_t logical[256],frag[244];
 size_t len=r1_encode(req,status,p,n,logical,sizeof(logical));
 if(!len) { ESP_LOGE(TAG,"MODEL_ENCODE_FAILED"); return; }
 if(tx_count+len/239+1>32) {ESP_LOGW(TAG,"TX_MODEL_QUEUE_FULL");return;}
 for(size_t i=0;i<=len/239;i++) {
  size_t f=r1_fragment(logical,len,i,frag,sizeof(frag));
  if(!queue_packet(l,tx2_handle,frag,f)) break;
 }
}
static void status_notify(struct link *l) {
 struct r1_model m={.module=1,.serial=notification_serial++,.command=0,.subcommand=1};
 uint8_t p[7]={R1_BATTERY_PERCENT,2,1,0,0,0,0};reply(l,&m,2,p,sizeof(p));
}
static bool save_blob(const char *key,const uint8_t *p,size_t n) {
 esp_err_t e=nvs_set_blob(settings,key,p,n);
 if(e==ESP_OK) e=nvs_commit(settings);
 if(e!=ESP_OK) ESP_LOGE(TAG,"NVS_SAVE key=%s error=%s",key,esp_err_to_name(e));
 return e==ESP_OK;
}
static void dispatch(struct link *l,const struct r1_model *m) {
 ESP_LOGI(TAG,"MODEL conn=%u module=%u version=%u cmd=%02x sub=%02x seq=%u status=%02x payload=%u",l->handle,m->module,m->module_version,m->command,m->subcommand,m->serial,m->status,(unsigned)m->payload_len);
 const uint8_t *p=m->payload;size_t n=m->payload_len;
 /* Result-code numbers beyond success have not been recovered. Unknown
    commands are logged, never ACKed as if implemented. */
 if(m->module!=1 || m->command!=0) { ESP_LOGW(TAG,"UNSUPPORTED_MODULE_OR_COMMAND");return; }
 if(m->subcommand!=8 && !l->phone) {ESP_LOGW(TAG,"PHONE_ROLE_REQUIRED conn=%u",l->handle);return;}
 switch(m->subcommand) {
 case 8: {
  if(n!=1 || p[0]!=1) { ESP_LOGW(TAG,"UNSUPPORTED_PAIR_ROLE");return; }
  if(!assign_role(l,true)) return;
  int rc=ble_gap_security_initiate(l->handle);
  ESP_LOGI(TAG,"PAIR_PHONE security_initiate=%d",rc);
  const uint8_t ok=0;reply(l,m,3,&ok,1);
  l->phone_auth_pending=true;l->auth_due_ms=now_ms()+100;
  advertise();break;
 }
 case 1: {
  uint8_t pstatus[7]={R1_BATTERY_PERCENT,2,1,0,0,0,0};
  reply(l,m,3,pstatus,sizeof(pstatus));break;
 }
 case 2: {
  uint8_t info[32]={0};
  memcpy(info,R1_APP_VERSION,strlen(R1_APP_VERSION));
  memcpy(info+16,R1_HW_VERSION,strlen(R1_HW_VERSION));
  reply(l,m,3,info,sizeof(info));break;
 }
 case 3: { const uint8_t wear=1;reply(l,m,3,&wear,1);break; }
 case 0x10: reply(l,m,3,(const uint8_t *)R1_SERIAL,15);break;
 case 4: if(n==12) {if(save_blob("user",p,n)) reply(l,m,3,NULL,0);} else ESP_LOGW(TAG,"BAD_USER_LENGTH");break;
 case 5: if(n==6) {if(save_blob("clock",p,n)) reply(l,m,3,NULL,0);} else ESP_LOGW(TAG,"BAD_TIME_LENGTH");break;
 case 7:
  if(n!=2) {ESP_LOGW(TAG,"BAD_TOUCH_LENGTH");break;}
  ESP_LOGI(TAG,"TOUCH_SWITCH selector=%u value=%u",p[0],p[1]);
  if(p[0]==2) source_enabled=p[1]!=0;
  reply(l,m,3,NULL,0);break;
 case 0x0a:
  if(n!=12) {ESP_LOGW(TAG,"BAD_ADVSTART_LENGTH expected=12");break;}
  reply(l,m,3,NULL,0);
  if(save_blob("targets",p,n)) memcpy(targets,p,n);
  log_bytes("ADVSTART_PEERS",l->handle,p,n);
  for(unsigned i=0;i<3;i++) if(links[i].used && links[i].glasses)
   ESP_LOGI(TAG,"TARGET_MATCH conn=%u result=%d",links[i].handle,r1_target_match(targets,links[i].peer));
  advertise();break;
 case 0x0e: case 0x0f: {
  const char *key=m->subcommand==0x0e?"health":"system";
  /* Old stock settings use status bit 1 as SET, not payload presence. */
  if((m->status&2) && n==12) {if(save_blob(key,p,n)) reply(l,m,3,NULL,0);}
  else if(!(m->status&2) && n==0) {uint8_t record[12]={0};size_t size=sizeof(record);nvs_get_blob(settings,key,record,&size);reply(l,m,3,record,sizeof(record));}
  else {ESP_LOGW(TAG,"BAD_SETTINGS_LENGTH");}
  break;
 }
 case 0x7e: ESP_LOGI(TAG,"PACKET_ACK");break;
 case 0x7f: reply(l,m,3,NULL,0);break;
 case 0x82:
  /* Reset only emulated peer targets; transport bonds require explicit local
     reset from the monitor, so accidental writes do not erase them. */
  if(n==1) {nvs_erase_key(settings,"targets");nvs_commit(settings);memset(targets,0,sizeof(targets));reply(l,m,3,NULL,0);advertise();}
  break;
 default: ESP_LOGW(TAG,"UNIMPLEMENTED_SYSTEM_SUBCOMMAND=%02x",m->subcommand);break;
 }
}
static void glasses_state(struct link *l) {
 if(!l->notify1) {l->initial_state_pending=true;return;}
 /* Legacy byte 5 is charging boolean, unlike the EUS state enum. */
 const uint8_t battery[6]={0,9,0x8b,0,R1_BATTERY_PERCENT,0};
 const uint8_t wear[5]={0,9,0x8c,0,1};
 if(tx_count>30) {l->initial_state_pending=true;return;}
 queue_packet(l,tx1_handle,battery,sizeof(battery));
 queue_packet(l,tx1_handle,wear,sizeof(wear));l->initial_state_pending=false;
}
static void legacy_dispatch(struct link *l,const uint8_t *p,size_t n) {
 struct r1_legacy_plan plan;
 if(!r1_legacy_parse(p,n,&plan)) {ESP_LOGW(TAG,"LEGACY_MALFORMED conn=%u",l->handle);return;}
 if(plan.action==R1_LEGACY_UNSUPPORTED) {ESP_LOGW(TAG,"LEGACY_UNIMPLEMENTED opcode=%02x",p[2]);return;}
 if(l->phone) {ESP_LOGW(TAG,"LEGACY_ON_PHONE conn=%u",l->handle);return;}
 int match=r1_target_match(targets,l->peer);
 ESP_LOGI(TAG,"LEGACY conn=%u opcode=%02x target=%d",l->handle,p[2],match);
#if R1_STRICT_TARGET_MATCH
 if(match==-2) {ESP_LOGW(TAG,"TARGET_REJECTED conn=%u",l->handle);return;}
#endif
 if(!assign_role(l,false)) return;
 if(plan.action==R1_LEGACY_PAIR_GLASSES) {
  ESP_LOGI(TAG,"G2_PAIR_ROLE_EVENT conn=%u (no synthetic 0x88 reply)",l->handle);
  glasses_state(l);advertise();return;
 }
 if(plan.action==R1_LEGACY_TOUCH_ENABLE) {
  l->touch_enabled=plan.enabled;
  ESP_LOGI(TAG,"G2_TOUCH conn=%u enabled=%d",l->handle,l->touch_enabled);
 } else if(plan.action==R1_LEGACY_HOLD_TIME) {
  ESP_LOGI(TAG,"G2_HOLD_TIME wire_be=%u (GPIO threshold remains 700ms)",plan.hold_time_be);
 } else if(plan.action==R1_LEGACY_GLASSES_STATUS) l->glasses_status=plan.glasses_status;
 if(l->notify1) queue_packet(l,tx1_handle,plan.response,plan.response_len);
 else ESP_LOGW(TAG,"NO_LEGACY_SUBSCRIPTION conn=%u",l->handle);
 advertise();
}
static int gatt_access(uint16_t conn,uint16_t attr,struct ble_gatt_access_ctxt *ctx,void *arg) {
 (void)attr;
 unsigned channel=(uintptr_t)arg;
 if(ctx->op!=BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_READ_NOT_PERMITTED;
 struct link *l=find_link(conn);if(!l) return BLE_ATT_ERR_UNLIKELY;
 uint8_t data[244];uint16_t len;
 if(OS_MBUF_PKTLEN(ctx->om)>sizeof(data) || ble_hs_mbuf_to_flat(ctx->om,data,sizeof(data),&len)) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
 log_bytes(channel==1?"RX_CH1":"RX_CH2",conn,data,len);
 if(channel==1) {
  legacy_dispatch(l,data,len);
  return 0;
 }
 int64_t now=esp_timer_get_time()/1000;
 if(l->rx.active && now-l->last_rx_ms>R1_REASSEMBLY_TIMEOUT_MS) {memset(&l->rx,0,sizeof(l->rx));ESP_LOGW(TAG,"RX_TIMEOUT");}
 l->last_rx_ms=now;
 int result=r1_receive(&l->rx,data,len);
 if(result<0) {ESP_LOGW(TAG,"REJECTED_EUS_FRAGMENT_OR_CRC");return 0;}
 if(result==1) {
  struct r1_model m;
  if(r1_decode(l->rx.data,l->rx.used,&m)) dispatch(l,&m);
  else {ESP_LOGW(TAG,"REJECTED_MODEL_OR_COMPACT_CRC");log_bytes("BAD_MODEL",conn,l->rx.data,l->rx.used);}
 }
 return 0;
}
static const struct ble_gatt_svc_def services[]={
 {.type=BLE_GATT_SVC_TYPE_PRIMARY,.uuid=&service_uuid.u,
  .characteristics=(struct ble_gatt_chr_def[]){
   {.uuid=&rx1_uuid.u,.access_cb=gatt_access,.arg=(void *)1,.flags=BLE_GATT_CHR_F_WRITE_NO_RSP},
   {.uuid=&tx1_uuid.u,.access_cb=gatt_access,.flags=BLE_GATT_CHR_F_NOTIFY,.val_handle=&tx1_handle},
   {.uuid=&rx2_uuid.u,.access_cb=gatt_access,.arg=(void *)2,.flags=BLE_GATT_CHR_F_WRITE_NO_RSP},
   {.uuid=&tx2_uuid.u,.access_cb=gatt_access,.flags=BLE_GATT_CHR_F_NOTIFY,.val_handle=&tx2_handle},
   {0}}}, {0}};
static void advertise(void) {
 unsigned count=0;for(unsigned i=0;i<3;i++) count+=links[i].used;
 bool phone=false,glass=false;
 for(unsigned i=0;i<3;i++) if(links[i].used) {phone|=links[i].phone;glass|=links[i].glasses;}
 if(count==3 || (phone && glass)) {
  if(ble_gap_adv_active()) {int rc=ble_gap_adv_stop();ESP_LOGI(TAG,"ADV_STOP roles_complete=%d rc=%d",phone&&glass,rc);}
  return;
 }
 if(ble_gap_adv_active()) return;
 struct ble_hs_adv_fields adv={0},scan={0};
 adv.flags=BLE_HS_ADV_F_DISC_GEN|BLE_HS_ADV_F_BREDR_UNSUP;
 adv.name=(uint8_t *)device_name;adv.name_len=strlen(device_name);adv.name_is_complete=1;
 adv.appearance=0x0240;adv.appearance_is_present=1;
 /* 0x5245 company + 6-byte LE identity + 15-byte emulated serial. */
 uint8_t manufacturer[23]={0x45,0x52};memcpy(manufacturer+2,identity_addr,6);memcpy(manufacturer+8,R1_SERIAL,15);
 scan.mfg_data=manufacturer;scan.mfg_data_len=sizeof(manufacturer);
 int rc=ble_gap_adv_set_fields(&adv);if(!rc) rc=ble_gap_adv_rsp_set_fields(&scan);
 if(rc) {ESP_LOGE(TAG,"ADV_FIELDS rc=%d",rc);return;}
 struct ble_gap_adv_params params={0};params.conn_mode=BLE_GAP_CONN_MODE_UND;params.disc_mode=BLE_GAP_DISC_MODE_GEN;params.itvl_min=0xa0;params.itvl_max=0xa0;
 rc=ble_gap_adv_start(own_addr_type,NULL,BLE_HS_FOREVER,&params,gap_event,NULL);
 ESP_LOGI(TAG,"ADVERTISING name=%s links=%u rc=%d",device_name,count,rc);
}
static int gap_event(struct ble_gap_event *event,void *arg) {
 (void)arg;
 switch(event->type) {
 case BLE_GAP_EVENT_CONNECT:
  if(!event->connect.status) {
   for(unsigned i=0;i<3;i++) if(!links[i].used) {
    memset(&links[i],0,sizeof(links[i]));links[i].used=true;links[i].touch_enabled=true;links[i].handle=event->connect.conn_handle;links[i].generation=++next_generation;
    struct ble_gap_conn_desc d;
    if(!ble_gap_conn_find(links[i].handle,&d)) {
     memcpy(links[i].peer,d.peer_id_addr.val,6);links[i].peer_type=d.peer_id_addr.type;
     links[i].encrypted=d.sec_state.encrypted;
     log_bytes("CONNECTED_PEER_LE",links[i].handle,d.peer_id_addr.val,6);
     log_bytes("CONNECTED_PEER_OTA_LE",links[i].handle,d.peer_ota_addr.val,6);
     ESP_LOGI(TAG,"PEER_TYPE conn=%u type=%u interval=%u latency=%u timeout=%u",links[i].handle,d.peer_id_addr.type,d.conn_itvl,d.conn_latency,d.supervision_timeout);
    }
    /* ESP-IDF's GATT client depends on the central role. Keep this emulator
       peripheral-only, as stock, and answer the peer's MTU exchange instead. */
    ESP_LOGI(TAG,"MTU_WAIT_PEER conn=%u preferred=247",links[i].handle);
    ESP_LOGI(TAG,"CONNECTED conn=%u",links[i].handle);break;
   }
  } else ESP_LOGW(TAG,"CONNECT_FAILED status=%d",event->connect.status);
  advertise();return 0;
 case BLE_GAP_EVENT_DISCONNECT: {
  uint16_t h=event->disconnect.conn.conn_handle;ESP_LOGI(TAG,"DISCONNECTED conn=%u reason=%d",h,event->disconnect.reason);
  struct link *l=find_link(h);if(l) memset(l,0,sizeof(*l));if(selected_conn==h) selected_conn=-1;advertise();return 0;
 }
 case BLE_GAP_EVENT_SUBSCRIBE: {
  struct link *l=find_link(event->subscribe.conn_handle);
  if(l) {
   if(event->subscribe.attr_handle==tx1_handle) {
    l->notify1=event->subscribe.cur_notify;
    /* Stock BAE8 group-A CCCD events assign the glasses role. */
    int match=r1_target_match(targets,l->peer);
    if(!R1_STRICT_TARGET_MATCH || match!=-2) {
     if(assign_role(l,false)) ESP_LOGI(TAG,"CCCD_GLASSES_ROLE_EVENT conn=%u target=%d",l->handle,match);
    } else ESP_LOGW(TAG,"CCCD_TARGET_REJECTED conn=%u",l->handle);
   }
   if(event->subscribe.attr_handle==tx2_handle) l->notify2=event->subscribe.cur_notify;
   ESP_LOGI(TAG,"SUBSCRIBE conn=%u ch1=%d ch2=%d",l->handle,l->notify1,l->notify2);
   advertise();
  } return 0;
 }
 case BLE_GAP_EVENT_ENC_CHANGE: {
  struct ble_gap_conn_desc d;
  if(!ble_gap_conn_find(event->enc_change.conn_handle,&d)) {
   ESP_LOGI(TAG,"SECURITY conn=%u status=%d encrypted=%u bonded=%u",d.conn_handle,event->enc_change.status,d.sec_state.encrypted,d.sec_state.bonded);
   struct link *l=find_link(d.conn_handle);if(l) {l->encrypted=d.sec_state.encrypted;l->auth_due_ms=now_ms()+100;memcpy(l->peer,d.peer_id_addr.val,6);l->peer_type=d.peer_id_addr.type;}
  }
  return 0;
 }
 case BLE_GAP_EVENT_REPEAT_PAIRING: {
  struct ble_gap_conn_desc d;
  ESP_LOGI(TAG,"REPEAT_PAIRING conn=%u",event->repeat_pairing.conn_handle);
  if(!ble_gap_conn_find(event->repeat_pairing.conn_handle,&d)) ble_store_util_delete_peer(&d.peer_id_addr);
  return BLE_GAP_REPEAT_PAIRING_RETRY;
 }
 case BLE_GAP_EVENT_MTU: ESP_LOGI(TAG,"MTU conn=%u value=%u",event->mtu.conn_handle,event->mtu.value);return 0;
 case BLE_GAP_EVENT_CONN_UPDATE: ESP_LOGI(TAG,"CONN_UPDATE conn=%u status=%d",event->conn_update.conn_handle,event->conn_update.status);return 0;
 case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE: ESP_LOGI(TAG,"PHY conn=%u status=%d tx=%u rx=%u",event->phy_updated.conn_handle,event->phy_updated.status,event->phy_updated.tx_phy,event->phy_updated.rx_phy);return 0;
 case BLE_GAP_EVENT_ADV_COMPLETE: advertise();return 0;
 default:return 0;
 }
}
static void touch_event(uint8_t type,uint8_t v0,uint8_t v1) {
 uint8_t p[11];uint32_t timestamp=(uint32_t)((esp_timer_get_time()*1024)/1000000);
 if(!r1_legacy_touch(type,v0,v1,timestamp,p,sizeof(p))) {ESP_LOGW(TAG,"INVALID_TOUCH_TYPE");return;}
 if(type!=8 && last_touch_tick && timestamp-last_touch_tick<100)
  ESP_LOGW(TAG,"TOUCH_WITHIN_100_TICKS G2 may suppress this event");
 last_touch_tick=timestamp;
 unsigned sent=0;
 for(unsigned i=0;i<3;i++) if(links[i].used && links[i].glasses && links[i].notify1 &&
   (selected_conn<0 || selected_conn==links[i].handle) && source_enabled && links[i].touch_enabled)
  sent+=queue_packet(&links[i],tx1_handle,p,sizeof(p));
 ESP_LOGI(TAG,"TOUCH type=%u v0=%u v1=%u queued=%u source=%d selected=%d",type,v0,v1,sent,source_enabled,selected_conn);
}
static void gesture(char c) {
 switch(c) {
 case 's':touch_event(1,0,0);break;
 case 'd':touch_event(2,0,0);break;
 case 'h':touch_event(0,0,0);break;
 case 'u':touch_event(4,1,1);break;
 case 'j':touch_event(5,1,1);break;
 case 'r':touch_event(8,0,0);break;
 default:break;
 }
}
static void dump_status(void) {
 ESP_LOGI(TAG,"STATUS probe=%s adv=%d tx_pending=%u source=%d selected=%d",R1_PROBE_VERSION,ble_gap_adv_active(),tx_count,source_enabled,selected_conn);
 log_bytes("TARGETS_LE_RAW",0,targets,sizeof(targets));
 for(unsigned i=0;i<3;i++) if(links[i].used) {
  struct link *l=&links[i];
  ESP_LOGI(TAG,"LINK conn=%u role=%s ch1=%d ch2=%d encrypted=%d mtu=%u touch=%d state=%02x target=%d",
   l->handle,l->phone?"phone":l->glasses?"glasses":"unknown",l->notify1,l->notify2,l->encrypted,ble_att_mtu(l->handle),l->touch_enabled,l->glasses_status,r1_target_match(targets,l->peer));
 }
}
static void console_dispatch(const char *text) {
 unsigned type,v0,v1;char trailing;
 if(strlen(text)==1 && strchr("sdhujr",text[0])) {gesture(text[0]);return;}
 if(!strcmp(text,"status")) {dump_status();return;}
 if(!strcmp(text,"auto")) {selected_conn=-1;ESP_LOGI(TAG,"SELECT auto");return;}
 if(sscanf(text,"select %u %c",&type,&trailing)==1 && type<=UINT16_MAX) {
  struct link *l=find_link((uint16_t)type);
  if(!l || !l->glasses) {ESP_LOGW(TAG,"SELECT_REQUIRES_GLASSES_LINK");return;}
  selected_conn=(int)type;ESP_LOGI(TAG,"SELECT conn=%u",type);return;
 }
 if(sscanf(text,"event %u %u %u %c",&type,&v0,&v1,&trailing)==3 && type<=255 && v0<=255 && v1<=255) {
  touch_event(type,v0,v1);return;
 }
 /* Explicit diagnostic notification; bypasses gesture gates, never runs
    automatically and never changes persistent bonds/targets. */
 if(!strncmp(text,"send ",5)) {
  char *end;const char *p=text+5;
  unsigned long conn=strtoul(p,&end,10);if(end==p || !isspace((unsigned char)*end) || conn>UINT16_MAX) goto invalid;
  p=end;unsigned long channel=strtoul(p,&end,10);if(end==p || !isspace((unsigned char)*end) || (channel!=1 && channel!=2)) goto invalid;
  struct link *l=find_link((uint16_t)conn);
  if(!l || (channel==1?!l->notify1:!l->notify2)) {ESP_LOGW(TAG,"SEND_REQUIRES_SUBSCRIPTION");return;}
  p=end;uint8_t bytes[244];size_t n=0;
  while(*p) {
   while(isspace((unsigned char)*p)) p++;
   if(!*p) break;
   if(!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1]) || n==sizeof(bytes)) goto invalid;
   char hex[3]={p[0],p[1],0};bytes[n++]=(uint8_t)strtoul(hex,NULL,16);p+=2;
  }
  if(!n) goto invalid;
  ESP_LOGI(TAG,"MANUAL_SEND conn=%lu channel=%lu len=%u",conn,channel,(unsigned)n);
  queue_packet(l,channel==1?tx1_handle:tx2_handle,bytes,n);return;
 }
 if(!strcmp(text,"help") || !strcmp(text,"?")) {
  ESP_LOGI(TAG,"Commands: status | s d h u j r | event TYPE V0 V1 | select CONN | auto | send CONN CHANNEL HEX");return;
 }
invalid:
 ESP_LOGW(TAG,"INVALID_COMMAND (help for syntax)");
}
static void tick(struct ble_npl_event *ev) {
 (void)ev;struct console_command command;
 /* One command per tick so manual bursts do not starve the BLE host. */
 if(xQueueReceive(input_queue,&command,0)==pdTRUE) console_dispatch(command.text);
 int64_t current=now_ms();
 for(unsigned i=0;i<3;i++) if(links[i].used) {
  struct link *l=&links[i];
  if(l->rx.active && current-l->last_rx_ms>R1_REASSEMBLY_TIMEOUT_MS) {memset(&l->rx,0,sizeof(l->rx));ESP_LOGW(TAG,"RX_TIMEOUT conn=%u",l->handle);}
  if(l->initial_state_pending && l->notify1) glasses_state(l);
  if(l->phone_auth_pending && l->encrypted && l->notify2 && current>=l->auth_due_ms && tx_count<=30) {
   struct r1_model m={.module=1,.serial=notification_serial++,.command=0,.subcommand=8};
   const uint8_t ok=0;reply(l,&m,2,&ok,1);status_notify(l);l->phone_auth_pending=false;
   ESP_LOGI(TAG,"PHONE_AUTH_NOTIFY conn=%u",l->handle);
  }
 }
#if R1_BUTTON_GPIO >= 0
 static bool last=false,held=false;static int64_t pressed_at,changed_at;static int clicks;
 int64_t now=esp_timer_get_time()/1000;bool down=!gpio_get_level(R1_BUTTON_GPIO);
 if(down!=last && now-changed_at>=30) {
  last=down;changed_at=now;
  if(down) {pressed_at=now;held=false;}
  else if(!held) {clicks++;if(clicks==2) {gesture('d');clicks=0;}}
  else gesture('r');
 }
 if(last && !held && now-pressed_at>=700) {gesture('h');held=true;clicks=0;}
 if(!last && clicks==1 && now-changed_at>=300) {gesture('s');clicks=0;}
#endif
 if(tx_count) {
  struct tx_packet *t=&tx_queue[tx_head];struct link *l=find_link(t->conn);bool remove=false;
  if(!l || l->generation!=t->generation || (t->attr==tx1_handle?!l->notify1:!l->notify2)) remove=true;
  else if(current-t->queued_ms>R1_TX_TIMEOUT_MS) {ESP_LOGW(TAG,"TX_TIMEOUT conn=%u len=%u mtu=%u",t->conn,t->len,ble_att_mtu(t->conn));remove=true;}
  else if(t->len>ble_att_mtu(t->conn)-3) { /* wait for MTU exchange until the deadline */ }
  else {
   struct os_mbuf *om=ble_hs_mbuf_from_flat(t->bytes,t->len);
   int rc=om?ble_gatts_notify_custom(t->conn,t->attr,om):BLE_HS_ENOMEM;
   if(!rc) {log_bytes(t->attr==tx1_handle?"TX_CH1":"TX_CH2",t->conn,t->bytes,t->len);remove=true;}
   else if(++t->retries>=10) {ESP_LOGW(TAG,"TX_FAILED conn=%u rc=%d",t->conn,rc);remove=true;}
  }
  if(remove) {tx_head=(tx_head+1)%32;tx_count--;}
 }
 ble_npl_callout_reset(&ticker,ble_npl_time_ms_to_ticks32(20));
}
static void on_sync(void) {
 int rc=ble_hs_util_ensure_addr(0);assert(!rc);
 rc=ble_hs_id_infer_auto(0,&own_addr_type);assert(!rc);
 /* Use the ESP32-S3's public identity consistently for name/manufacturer. */
 rc=ble_hs_id_copy_addr(own_addr_type,identity_addr,NULL);assert(!rc);
 snprintf(device_name,sizeof(device_name),"EVEN R1_%02X%02X%02X",identity_addr[3],identity_addr[2],identity_addr[1]);
 ble_svc_gap_device_name_set(device_name);ble_svc_gap_device_appearance_set(0x0240);
 ESP_LOGI(TAG,"IDENTITY name=%s app=%s serial=%s",device_name,R1_APP_VERSION,R1_SERIAL);
 log_bytes("IDENTITY_ADDR_LE",0,identity_addr,6);
 size_t n=sizeof(targets);if(nvs_get_blob(settings,"targets",targets,&n)==ESP_OK && n==sizeof(targets)) log_bytes("SAVED_ADVSTART_PEERS",0,targets,n);
 else memset(targets,0,sizeof(targets));
 advertise();ble_npl_callout_reset(&ticker,ble_npl_time_ms_to_ticks32(20));
}
static void on_reset(int reason) {ESP_LOGE(TAG,"HOST_RESET reason=%d",reason);memset(links,0,sizeof(links));tx_count=0;tx_head=0;selected_conn=-1;}
static void host_task(void *p) {(void)p;nimble_port_run();nimble_port_freertos_deinit();}
void app_main(void) {
 /* Never auto-erase NVS on failure: persisted bonds must not silently vanish. */
 ESP_ERROR_CHECK(nvs_flash_init());ESP_ERROR_CHECK(nvs_open("r1test",NVS_READWRITE,&settings));
 _Static_assert(sizeof(R1_SERIAL)==16,"R1_SERIAL must be 15 ASCII bytes");
 _Static_assert(sizeof(R1_APP_VERSION)<=16,"app version exceeds slot");
 _Static_assert(sizeof(R1_HW_VERSION)<=16,"hardware version exceeds slot");
 input_queue=xQueueCreate(8,sizeof(struct console_command));assert(input_queue);
#if R1_BUTTON_GPIO >= 0
 gpio_config_t cfg={.pin_bit_mask=1ULL<<R1_BUTTON_GPIO,.mode=GPIO_MODE_INPUT,.pull_up_en=GPIO_PULLUP_ENABLE};ESP_ERROR_CHECK(gpio_config(&cfg));
#endif
 ESP_ERROR_CHECK(nimble_port_init());
 ble_hs_cfg.sync_cb=on_sync;ble_hs_cfg.reset_cb=on_reset;
 ble_hs_cfg.sm_bonding=1;ble_hs_cfg.sm_mitm=0;ble_hs_cfg.sm_sc=0;ble_hs_cfg.sm_io_cap=BLE_HS_IO_NO_INPUT_OUTPUT;
 ble_hs_cfg.sm_our_key_dist=BLE_SM_PAIR_KEY_DIST_ENC|BLE_SM_PAIR_KEY_DIST_ID;
 ble_hs_cfg.sm_their_key_dist=BLE_SM_PAIR_KEY_DIST_ENC|BLE_SM_PAIR_KEY_DIST_ID;
 ble_hs_cfg.store_status_cb=ble_store_util_status_rr;
 ble_svc_gap_init();ble_svc_gatt_init();ble_store_config_init();
 assert(!ble_att_set_preferred_mtu(247));
 assert(!ble_gatts_count_cfg(services));assert(!ble_gatts_add_svcs(services));
 ble_npl_callout_init(&ticker,nimble_port_get_dflt_eventq(),tick,NULL);
 nimble_port_freertos_init(host_task);
 ESP_LOGI(TAG,"EXPERIMENTAL R1 compatibility firmware; registration/G2 control NOT verified");
 ESP_LOGI(TAG,"PROBE_VERSION=%s; type help then Enter",R1_PROBE_VERSION);
 struct console_command command={0};size_t used=0;bool overflow=false;
 while(1) {
  int c=getchar();
  if(c==EOF) {clearerr(stdin);vTaskDelay(pdMS_TO_TICKS(20));continue;}
  if(c=='\r' || c=='\n') {
   if(overflow) ESP_LOGW(TAG,"CONSOLE_LINE_TOO_LONG");
   else if(used) {command.text[used]=0;if(xQueueSend(input_queue,&command,0)!=pdTRUE) ESP_LOGW(TAG,"CONSOLE_QUEUE_FULL");}
   used=0;overflow=false;
  } else if(c==8 || c==127) {if(used && !overflow) used--;}
  else if(!overflow) {if(used<sizeof(command.text)-1) command.text[used++]=(char)c;else overflow=true;}
 }
}
