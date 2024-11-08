#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

char * diff_with_file (const char  *file,
                       char        *text,
                       gssize       len,
                       GError     **error);

char * diff_bytes (GBytes  *b1,
                   GBytes  *b2,
                   GError **error);

gboolean file_has_prefix (const char  *file,
                          const char  *str,
                          GError     **error);

void print_attribute (PangoAttribute *attr,
                      GString        *string);

void print_attributes (GSList        *attrs,
                       GString       *string);

void print_attr_list (PangoAttrList  *attrs,
                      GString        *string);

const char *get_script_name (GUnicodeScript s);

PangoFontMap *get_font_map_with_cantarell (void);

#endif
