
#ifndef __SERPHONE_INTERFACE_H_
#define __SERPHONE_INTERFACE_H_

struct _SerphoneCoreVTable;
typedef struct _SerphoneVTable SerphoneCoreVTable;
class ServiceCore; 


ServiceCore *serphone_core_new(const SerphoneCoreVTable *vtable,
						const char *config_path, const char *factory_config, void* userdata);
void serphone_core_destroy(ServiceCore *lc);

void PrintConsole(char * fmt,...);

#endif