#include <ArduinoJson.h>

#define HWSERIAL Serial4

namespace ledtopiaController {
    void setup() {
        HWSERIAL.begin(115200);
    }

    void sendState(int state, bool isPortalOpen, int starDelay) {
        DynamicJsonDocument doc(200);
        doc["State"] = state;
        doc["isPortalOpen"] = isPortalOpen;
        doc["starDelay"] = starDelay;
        char data[200];
        serializeJson(doc, data);
        HWSERIAL.write(data);
        HWSERIAL.write("\n");
    }
}