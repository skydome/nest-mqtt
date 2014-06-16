#include <NanodeUNIO.h>
#include <NanodeUIP.h>
#include <NanodeMQTT.h>
#include <RF22ReliableDatagram.h>
#include <RF22.h>
#include <SPI.h>

NanodeMQTT mqtt(&uip);

//#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 0

// Class to manage message delivery and receipt, using the driver declared above
RF22ReliableDatagram manager(SERVER_ADDRESS);
const byte macaddr[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
const char topic[] = "skydome/7469692d3031";
int a = -1;
int b = -1;
int c = -1;
int d = -1;
const int DOT = 46;
const int EOI = 0;

static void resolv_found(char *name, uip_ipaddr_t *addr) {
  char address[17];
  // Serial.println(name);
  int temp = 0;
  uip.format_ipaddr(address, addr);
  for (int i = 0; i < 17; i++) {

    if (int(address[i]) != DOT && int(address[i]) != EOI && ((int(address[i]) - 48) >= 0 && (int(address[i]) - 48) <= 9) ) {
      temp = (temp + (int(address[i]) - 48)) * 10 ;
    } else {
      if (a == -1)
        a = temp / 10;
      else if (b ==  -1)
        b = temp / 10;
      else if (c ==  -1)
        c = temp / 10;
      else if (d ==  -1)
        d = temp / 10;
      temp = 0;
    }

    if (int(address[i]) == EOI)
      break;

  }

  //  Serial.println("Connecting to : ");
  //  Serial.print(a);
  //  Serial.print(".");
  //  Serial.print(b);
  //  Serial.print(".");
  //  Serial.print(c);
  //  Serial.print(".");
  //  Serial.println(d);
  mqtt.set_server_addr(a, b, c, d);
  mqtt.connect();

  /// connected message
  uint8_t data[10] = { macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5],SERVER_ADDRESS, 255, 255, 255};
  uint8_t len = 10;
  publish(data, len);
  Serial.println(F("setup() done"));
}

void dhcp_status(int s, const uip_ipaddr_t *dnsaddr) {
  char buf[20] = "IP:";
  if (s == DHCP_STATUS_OK) {
    resolv_conf(dnsaddr);
    uip.get_ip_addr_str(buf + 3);
    //Serial.println(buf);
    uip.query_name("api.skydome.io");
  }
}

void setup() {
  char buf[20] PROGMEM;
  Serial.begin(9600);
  /// RF initialization must be before network initialization, otherwise RF will not initialize!
  if (manager.init())
    Serial.println(F("init success"));
  else
    Serial.println(F("init failed"));

  uip.init(macaddr);
  uip.get_mac_str(buf);
  Serial.println(buf);
  uip.wait_for_link();
  Serial.println(F("Link is up"));
  uip.init_resolv(resolv_found);
  uip.start_dhcp(dhcp_status);
}

void publish(uint8_t data[], uint8_t len ) {
  if (mqtt.connected()) {
    mqtt.publish(topic, data, len);
  } else {
    mqtt.connect();
  }
}

void loop() {
  uip.poll();

  uint8_t buf[2] PROGMEM;
  if (manager.available()) {
    uint8_t len = 2;
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from)) {
      uint8_t data[10] = { macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], SERVER_ADDRESS, from, buf[0], buf[1]};
      uint8_t len = 10;
      publish(data, len);
      // Send a reply back to the originator client
      if (!manager.sendtoWait(buf, 2, from)) {
        //// alarm to a red led !!!
      }
    }
  }
}
