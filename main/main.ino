void printFloat(float f, uint8_t C = 0x00) {
  uint8_t data[] = { 0x00 , 0x00, 0x00, C };
  String s = String(f);
  int tip = String(int(f)).length();
  int di = s.indexOf('.');

  if (C == 0x00) { 
    String ns = delzeros(String(sr(f, 4 - tip)));
    Serial.println(ns);
    ns.remove(ns.indexOf('.'), 1);
    int N = ns.length();
    int c = 0;
    if (N > 0) {
      data[3] = display.encodeDigit(int(ns[N - 1])); c++;
    }
    if (N > 1) {
      data[2] = display.encodeDigit(int(ns[N - 2])); c++;
    }
    if (N > 2) {
      data[1] = display.encodeDigit(int(ns[N - 3])); c++;
    }
    if (N > 3) {
      data[0] = display.encodeDigit(int(ns[N - 4])); c++;
    }
    Serial.println("di: " + String(di) + "; c: " + String(c));
    if (di > 0)
      data[ di + 3 - c] += 0x80;
    display.setSegments(data);
  }
  else {
    String ns = String(sr(f, 4 - di));
    ns.remove(ns.indexOf('.'), 1);
    int N = ns.length();
    if (N > 0) {
      data[2] = display.encodeDigit(int(ns[2]));
    }
    if (N > 1) {
      data[1] = display.encodeDigit(int(ns[1]));
    }
    if (N > 2) {
      data[0] = display.encodeDigit(int(ns[0]));
    }
    if (di > 0)
      data[ di - 1 ] += 0x80;
    display.setSegments(data);
  }
}
