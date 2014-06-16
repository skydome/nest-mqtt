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

static void resolv_found(char *name, uip_ipaddr_t *addr) {
  char buf[30] = ": addr=";
  Serial.println(name);
  uip.format_ipaddr(buf + 7, addr);
  Serial.println(buf);
}

void dhcp_status(int s, const uip_ipaddr_t *dnsaddr) {
  char buf[20] = "IP:";
  if (s == DHCP_STATUS_OK) {
    resolv_conf(dnsaddr);
    uip.get_ip_addr_str(buf + 3);
    Serial.println(buf);
    uip.query_name("api.skydome.io");
  }
}

void setup() {
  char buf[20] PROGMEM;

  // NanodeUNIO unio(NANODE_MAC_DEVICE);

  Serial.begin(9600);
  /// RF initialization must be before network initialization, otherwise RF will not initialize!
  if (manager.init())
    Serial.println(F("init success"));
  else
    Serial.println(F("init failed"));
  //Serial.println("UIP test");

  //unio.read(macaddr, NANODE_MAC_ADDRESS, 6);
  uip.init(macaddr);
  uip.get_mac_str(buf);
  Serial.println(buf);
  uip.wait_for_link();
  Serial.println(F("Link is up"));
  uip.init_resolv(resolv_found);
  uip.start_dhcp(dhcp_status);

  // FIXME: resolve using DNS instead 107.170.134.171
  mqtt.set_server_addr(107, 170, 134, 171);
  mqtt.connect();


  Serial.println(F("setup() done"));
}



void loop() {
  uip.poll();

  uint8_t buf[2] PROGMEM;
  if (manager.available()) {
    uint8_t len = 2;
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
//      Serial.print(F("got request from : 0x"));
//      Serial.print(from);
//      Serial.print(": ");
//      Serial.print(buf[0]);
//      Serial.print(": ");
//      Serial.println(buf[1]);
      uint8_t data[9] = { macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], from, buf[0], buf[1]};
      uint8_t len = 9;
      if (mqtt.connected()) {
        //Serial.println(F("Publishing..."));
        mqtt.publish("skydome", data, len);
       // Serial.println(F("Published."));
      } else {
        //Serial.println(F("Connecting.."));

        mqtt.connect();
      }
      // Send a reply back to the originator client
      if (!manager.sendtoWait(buf, 2, from))
        //Serial.println(F("sendtoWait failed"));
    }
  }
}
