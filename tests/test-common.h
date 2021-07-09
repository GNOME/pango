#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

char * diff_with_file (const char  *file,
                       char        *text,
                       gssize       len,
                       GError     **error);

gboolean file_has_prefix (const char  *file,
                          const char  *str,
                          GError     **error);

void print_attribute (PangoAttribute *attr,
                      GString        *string);

void print_attributes (GSList        *attrs,
                       GString       *string);

void print_attr_list (PangoAttrList  *attrs,
                      GString        *string);

PangoAttribute *
attribute_from_string (const char *string);

PangoAttrList *
attributes_from_string (const char *string);

const char *get_script_name (GUnicodeScript s);


#endif
