#include <kernel/config.h>
#include <kernel/simfs.h>
#include <stdlib.h>
#include <vector.h>
#include <ctype.h>
#include <string.h>

struct ConfigEntry {
  // The memory for name and value are allocated by malloc.
  const char* name;
  const char* value;
};

// the memory is never freed
vector<ConfigEntry> entryList_;

// TODO a hash table will be much better
static const char *get_config(const char* name) {
  for (auto& entry : entryList_) {
    if (strcmp(name, entry.name) == 0) {
      return entry.value;
    }
  }
  assert(false && "config not found");
}

const char *get_wireless_network_name() {
  return get_config("wireless_network_name");
}

const char *get_wireless_network_password() {
  return get_config("wireless_network_password");
}

void kernel_config_init() {
  int filesize;
  char *filebuf = (char*) SimFs::get().readFile("/sos.cfg", &filesize);
  char *cur = filebuf;
  char *bufend = filebuf + filesize;
  // spaces will separate k/v pairs.
  while (cur != bufend) {
    // handle a new line
    if (isspace(*cur)) {
      ++cur;
      continue;
    }
    char *eq = cur;
    while (eq != bufend && !isspace(*eq) && *eq != '=') {
      ++eq;
    }
    assert(eq != bufend && *eq == '='); // found eq
    char *kvend = eq + 1;
    while (kvend != bufend && !isspace(*kvend)) {
      ++kvend;
    }

    ConfigEntry entry;
    entry.name = strndup(cur, eq - cur);
    entry.value = strndup(eq + 1, kvend - eq - 1);
    entryList_.push_back(entry);
    cur = kvend;
    if (cur != bufend) {
      assert(isspace(*cur));
      ++cur;
    }
  }

  free(filebuf);
}
