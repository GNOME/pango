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

void print_attribute (Pango2Attribute *attr,
                      GString        *string);

void print_attributes (GSList        *attrs,
                       GString       *string);

void print_attr_list (Pango2AttrList  *attrs,
                      GString        *string);

const char *get_script_name (GUnicodeScript s);

void    install_fonts      (void);
void    dump_fonts         (void);

#endif
