unsigned char interrupts[] = {
  0x10, 0x00, 0xe0, 0x00, 0x18, 0x01, 0x08, 0x00, 0xe9, 0x00, 0x01, 0x00,
  0x90, 0x90, 0x90, 0x90, 0x66, 0x3d, 0x9e, 0x00, 0x74, 0x21, 0x66, 0x3d,
  0xe4, 0x00, 0x74, 0x69, 0x3d, 0x77, 0xf7, 0x01, 0x00, 0x0f, 0x84, 0xc8,
  0x00, 0x00, 0x00, 0x3d, 0x07, 0xf7, 0x01, 0x00, 0x0f, 0x84, 0xc6, 0x00,
  0x00, 0x00, 0xe7, 0x00, 0x48, 0x0f, 0x07, 0x0f, 0x01, 0xcb, 0x56, 0x51,
  0x52, 0x48, 0x81, 0xff, 0x02, 0x10, 0x00, 0x00, 0x75, 0x1b, 0xb9, 0x00,
  0x01, 0x00, 0xc0, 0x89, 0xf0, 0x48, 0xc1, 0xee, 0x20, 0x89, 0xf2, 0x0f,
  0x30, 0x48, 0x31, 0xc0, 0x5a, 0x59, 0x5e, 0x0f, 0x01, 0xca, 0x48, 0x0f,
  0x07, 0x48, 0x81, 0xff, 0x03, 0x10, 0x00, 0x00, 0x75, 0x16, 0xb9, 0x00,
  0x01, 0x00, 0xc0, 0x0f, 0x32, 0x48, 0xc1, 0xe2, 0x20, 0x48, 0x09, 0xc2,
  0x48, 0x89, 0x06, 0x48, 0x31, 0xc0, 0xeb, 0xd8, 0x66, 0xe7, 0x00, 0xeb,
  0xd3, 0x0f, 0x01, 0xcb, 0x53, 0x51, 0x52, 0x66, 0x0f, 0xb6, 0x1c, 0x25,
  0x0a, 0x20, 0x00, 0x00, 0x66, 0x85, 0xdb, 0x74, 0x36, 0x48, 0x83, 0xec,
  0x40, 0xb8, 0x09, 0x00, 0x00, 0x00, 0x48, 0x89, 0xe3, 0xb9, 0x00, 0x00,
  0x00, 0x00, 0x0f, 0x01, 0xc1, 0x48, 0x83, 0xc4, 0x40, 0x48, 0x85, 0xc0,
  0x75, 0x19, 0x48, 0x8b, 0x0b, 0x48, 0x8b, 0x53, 0x08, 0x48, 0x89, 0x0e,
  0x48, 0x89, 0x56, 0x08, 0x5a, 0x59, 0x5b, 0x0f, 0x01, 0xca, 0x31, 0xc0,
  0x48, 0x0f, 0x07, 0xb8, 0xe4, 0x00, 0x00, 0x00, 0x5a, 0x59, 0x5b, 0x0f,
  0x01, 0xca, 0x66, 0xe7, 0x00, 0x48, 0x0f, 0x07, 0xb8, 0x60, 0x00, 0x00,
  0x00, 0x66, 0xe7, 0x00, 0xc3, 0xb8, 0xe0, 0x00, 0x00, 0x00, 0xc3, 0x0f,
  0x20, 0xd8, 0x0f, 0x22, 0xd8, 0x48, 0x0f, 0x07, 0x48, 0x0f, 0x07, 0x57,
  0x0f, 0x20, 0xd7, 0x66, 0xe7, 0x8e, 0x0f, 0x01, 0x3f, 0x5f, 0x48, 0x83,
  0xc4, 0x08, 0x48, 0xcf, 0x66, 0xe7, 0xa1, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x80, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x81, 0x48, 0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x82, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x83, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x84, 0x48, 0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x85, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x86, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x87, 0x48, 0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x88, 0xeb,
  0xa9, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x89, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x8a, 0xeb, 0x99, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x8b, 0xeb,
  0x91, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x8c, 0xeb, 0x89, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x8d, 0xeb, 0x81, 0x90, 0x90, 0x90, 0xe9, 0x6e, 0xff, 0xff,
  0xff, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x8f, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x90, 0x48, 0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x91, 0xe9,
  0x5e, 0xff, 0xff, 0xff, 0x66, 0xe7, 0x92, 0x48, 0xcf, 0x90, 0x90, 0x90,
  0x66, 0xe7, 0x93, 0x48, 0xcf, 0x90, 0x90, 0x90, 0x66, 0xe7, 0x94, 0x48,
  0xcf, 0x90, 0x90, 0x90, 0xe9, 0x47, 0xff, 0xff, 0xff
};
unsigned int interrupts_len = 453;
