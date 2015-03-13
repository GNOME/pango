#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

char * diff_with_file (const char  *file,
                       char        *text,
                       gssize       len,
                       GError     **error);

void print_attribute (PangoAttribute *attr,
                      GString        *string);

void print_attributes (GSList        *attrs,
                       GString       *string);

void print_attr_list (PangoAttrList  *attrs,
                      GString        *string);

GSList *attr_list_to_list (PangoAttrList *attrs);



#endif
