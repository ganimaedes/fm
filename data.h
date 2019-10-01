#ifndef DATA_H
#define DATA_H

#include <string.h>

int size_allocated = 0;

int sz_allocated = 0;
int sz = 10;
char *entries[] = { "entry0", "entry1", "entry2", "entry3", "entry4",
                    "entryh", "entry6", "entry7", "entry8", "entry9" };

char *entries_types[] = { "file", "file", "file", "file", "file",
                         "file", "file", "file", "file", "directory" };

int s_sz_allocated = 0;
int s_sz = 15;
char *s_entries[] = { "s_entry0", "s_entry1", "s_entry2", "s_entry3", "s_entry4", 
                      "s_entry5", "s_entry6", "s_entry7", "s_entry8", "s_entry9", 
                      "s_entry10", "s_entry11", "s_entry12", "s_entry13", "s_entry14" };

char *s_entries_types[] = { "file", "file", "file", "file", "file", 
                            "file", "file", "file", "file", "file", 
                            "file", "file", "file", "directory", "file" };

int t_sz_allocated = 0;
int t_sz = 20;
char *t_entries[] = { "t_entry0", "t_entry1", "t_entry2", "t_entry3", "t_entry4", 
                      "t_entry5", "t_entry6", "t_entry7", "t_entry8", "t_entry9", 
                      "t_entry10", "t_entry11", "t_entry12", "t_entry13", "t_entry14", 
                      "t_entry15", "t_entry16", "t_entry17", "t_entry18", "t_entry19" };

char *t_entries_types[] = { "file", "file", "file", "file", "file", 
                            "file", "file", "file", "file", "file", 
                            "file", "file", "file", "file", "file", 
                            "file", "file", "file", "file", "file" };

#endif  // DATA_H
