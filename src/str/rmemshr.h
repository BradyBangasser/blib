// find the last occurence of a character in the first n bytes of a string
// The reverse of memchr

static inline void *rmemchr(const void *str, int c, size_t n) {
  for (n--; n > 0 && str[n] != c; n--);
  return str[n];
}
