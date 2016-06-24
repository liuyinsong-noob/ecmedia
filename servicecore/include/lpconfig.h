 
#ifndef LPCONFIG_H
#define LPCONFIG_H

/**
 * The LpConfig object is used to manipulate a configuration file.
 * 
 * @ingroup misc
 * The format of the configuration file is a .ini like format:
 * - sections are defined in []
 * - each section contains a sequence of key=value pairs.
 *
 * Example:
 * @code
 * [sound]
 * echocanceler=1
 * playback_dev=ALSA: Default device
 *
 * [video]
 * enabled=1
 * @endcode
**/
typedef struct _LpConfig LpConfig;

#ifdef __cplusplus
extern "C" {
#endif

LpConfig * lp_config_new(const char *filename);
int lp_config_read_file(LpConfig *lpconfig, const char *filename);
/**
 * Retrieves a configuration item as a string, given its section, key, and default value.
 * 
 * @ingroup misc
 * The default value string is returned if the config item isn't found.
**/
const char *lp_config_get_string(LpConfig *lpconfig, const char *section, const char *key, const char *default_string);
int lp_config_read_file(LpConfig *lpconfig, const char *filename);
/**
 * Retrieves a configuration item as an integer, given its section, key, and default value.
 * 
 * @ingroup misc
 * The default integer value is returned if the config item isn't found.
**/
int lp_config_get_int(LpConfig *lpconfig,const char *section, const char *key, int default_value);
int lp_config_read_file(LpConfig *lpconfig, const char *filename);
/**
 * Retrieves a configuration item as a float, given its section, key, and default value.
 * 
 * @ingroup misc
 * The default float value is returned if the config item isn't found.
**/
float lp_config_get_float(LpConfig *lpconfig,const char *section, const char *key, float default_value);
/**
 * Sets a string config item 
 *
 * @ingroup misc
**/
void lp_config_set_string(LpConfig *lpconfig,const char *section, const char *key, const char *value);
/**
 * Sets an integer config item
 *
 * @ingroup misc
**/
void lp_config_set_int(LpConfig *lpconfig,const char *section, const char *key, int value);
/**
 * Sets a float config item
 *
 * @ingroup misc
**/
void lp_config_set_float(LpConfig *lpconfig,const char *section, const char *key, float value);	
/**
 * Writes the config file to disk.
 * 
 * @ingroup misc
**/
int lp_config_sync(LpConfig *lpconfig);
/**
 * Returns 1 if a given section is present in the configuration.
 *
 * @ingroup misc
**/
int lp_config_has_section(LpConfig *lpconfig, const char *section);
/**
 * Removes every pair of key,value in a section and remove the section.
 *
 * @ingroup misc
**/
void lp_config_clean_section(LpConfig *lpconfig, const char *section);
/*tells whether uncommited (with lp_config_sync()) modifications exist*/
int lp_config_needs_commit(const LpConfig *lpconfig);
void lp_config_destroy(LpConfig *cfg);
	
#ifdef __cplusplus
}
#endif

#endif
