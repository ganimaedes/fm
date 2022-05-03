#include "props.h"
#include <X11/Xlib.h>

#define ATOM(a, True_False) XInternAtom(foreground_dpy, #a, (True_False))

void close_display()
{
  if (foreground_dpy == NULL) {
    return;
  }
  XCloseDisplay(foreground_dpy);
  foreground_dpy = NULL;
}

void error(const char *msg, ...)
{
  va_list args = { 0 };
  fflush(stdout);
  fflush(stderr);
  fprintf(stderr, "Error: ");
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");
  close_display();
  exit(EXIT_FAILURE);
}

void setup_mapping(Display *dis)
{
  PropertyRec *p = windowPropTable;
  int n = sizeof(windowPropTable) / sizeof(PropertyRec);
  for (; --n >= 0; ++p) {
    if (!p->atom) {
      p->atom = XInternAtom(dis, p->name, True);
      if (p->atom == None) {
        continue;
      }
    }
    if (!property_formats) {
      if (!(property_formats = calloc(n + 1, sizeof *property_formats))) {
        error("Error calloc property_formats");
      }
      property_formats->capacity       = n + 1;
      property_formats->n_elements     = 0;
      property_formats->value          = 0;
    }
    property_formats[property_formats->n_elements++].value = p->atom;
  }
}

char *format_atom(Atom atom, Display *dis)
{
  char *name = NULL;
  name = XGetAtomName(dis, atom);
  if (!name) {
    snprintf(formatting_buffer, sizeof(formatting_buffer),
        "undefined atom # 0x%lx", atom);
  } else {
    int namelen = strlen(name);
    if (namelen > MAXSTR) {
      namelen = MAXSTR;
    }
    memcpy(formatting_buffer, name, namelen);
    formatting_buffer[namelen] = '\0';
    memcpy(formatting_copy, name, namelen);
    formatting_copy[namelen] = '\0';
    XFree(name);
  }
  return formatting_buffer;
}

void init_array(Atom_Prop *atom_prop, size_t inital_size)
{
  if (!(atom_prop->status = malloc((inital_size + 1) * sizeof *atom_prop->status))) {
    fprintf(stderr, "Error calloc atom->status\n");
  }
  atom_prop->total_len = 0;
}

int add_element(Atom_Prop *atom_prop, char *element)
{
  size_t len_element = strlen(element);
  if (len_element > 0) {
    atom_prop->total_len += len_element;
    void *tmp = NULL;
    tmp = realloc(atom_prop->status, (atom_prop->total_len + 1) * sizeof(char *));
    if (!tmp) {
      fprintf(stderr, "Error realloc atom->status\n");
      return 0;
    }
    atom_prop->status = tmp;
    memcpy(&atom_prop->status[atom_prop->total_len - len_element], element, len_element);
    atom_prop->status[atom_prop->total_len] = '\0';
#if defined(SHOW_ATOMS)
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    printf("len_element %lu, atom_prop->total_len = %d\natom_prop->status[%d]: %s\n",
        len_element, atom_prop->total_len, atom_prop->total_len, atom_prop->status);
#endif
    return 1;
  }
  return 0;
}

void display_property2(Atom_Prop *atom_prop, Properties *properties, Display *dis, int use_dyn)
{
  long i          = 0;
  char *name      = NULL;
#if defined(V_DEBUG)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  printf("properties->n_elements = %d\n", properties->n_elements);
#endif
  if (use_dyn) {
    init_array(atom_prop, 30);
  }
  for (; i < properties->n_elements; ++i) {
    //name = XGetAtomName(foreground_dpy, properties[i].value);
    name = XGetAtomName(dis, properties[i].value);
#if defined(V_DEBUG)
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    fprintf(stdout, "name: %s\n", name);
#endif
    if (use_dyn) {
      add_element(atom_prop, name);
    } else {
      int namelen = strlen(name);
      memcpy(formatting_buffer, name, namelen);
      formatting_buffer[namelen] = '\0';
    }
    XFree(name);
  }
}

long extract_value(const char **data, int *length, int size)
{
  long value;
  if (size != 32) {
    error("size isn't 32");
  }
  value = *(const long *) *data;
  *data += sizeof(long);
  *length -= sizeof(long);
#if defined(V_DEBUG)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  fprintf(stdout, "value = %lu, **data = %c, *length = %d\n", value, **data, *length);
#endif
  return value;
}

Properties *break_down_property(const char *data, int length, Atom type, int size)
{
  int n_properties_to_add = 0;
  int length_cpy          = length;
  if (size == 32) {
    while (length_cpy >= size / 8) { ++n_properties_to_add; length_cpy -= sizeof(long); }
  }
#if defined(V_DEBUG)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  fprintf(stdout, "n_properties_to_add = %d | length = %d\n", n_properties_to_add, length);
#endif

  Properties *properties = malloc(n_properties_to_add * sizeof *properties);
  if (!properties) {
    error("Error malloc properties");
  }
  properties->capacity   = n_properties_to_add;
  properties->n_elements = 0;
  properties->value      = 0;

  while (length >= size / 8 && properties->n_elements <= properties->capacity) {
    properties[properties->n_elements++].value = extract_value(&data, &length, size);
  }
  return properties;
}


void show_prop2(Atom_Prop *atom_prop, const char *prop, Window *top_win, Display *dis, int use_dyn)
{
  int size                  = 0;
  long length               = 0;
  unsigned long nitems      = 0;
  unsigned long nbytes      = 0;
  unsigned long bytes       = 0;
  int status                = 0;
  unsigned char *data       = NULL;
  Atom type;
  Atom atom  = XInternAtom(dis, prop, True);
  if (atom == None) {
    printf(":  no such atom on any window.\n");
    return;
  }
  status = XGetWindowProperty(dis, *top_win, atom, 0, (MAXSTR + 3) / 4,
                              False, AnyPropertyType, &type,
                              &size, &nitems, &bytes,
                              &data);
  if (status == BadWindow || status != Success) {
    error("window id # 0x%lx does not exists!", target_win);
  }
  if (size == 32) {
    nbytes = sizeof(long);
  } else {
    error("actual_format ie size: %d isn't 32", size);
  }
  length = MIN(nitems * nbytes, MAXSTR);
  if (!size) {
    puts(":  not found.");
    return;
  }
  Properties *properties = break_down_property((const char *)data, (int)length, type, size);
  //Display_Property3(atom_prop, properties, dis, use_dyn);
  display_property2(atom_prop, properties, dis, use_dyn);
  XFree(data);
  data = NULL;
  free(properties);
  properties = NULL;
}

void show_properties(Atom_Prop *atom_prop, Window *top_win, Display *dis, int use_dyn)
{
  Atom *atoms     = NULL;
  char *name      = NULL;
  int count, i;
  if (/*target_win != -1 ||*/ *top_win != -1) {
    atoms = XListProperties(dis, *top_win, &count);
    for (i = 0; i < count; ++i) {
#if defined(V_DEBUG)
      fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
      printf("tmp_window = 0x%lx\n\t", *top_win);
      printf("atom = %lu\n", atoms[i]);
#endif
      //name = Format_Atom2(atoms[i], dis);
      name = format_atom(atoms[i], dis);
      if (strstr(name, "_NET_WM_STATE")) {
        show_prop2(atom_prop, name, top_win, dis, use_dyn);
        break;
      }
    }
    XFree(atoms);
  }
}

Window Select_Window_Args(int *rargc, char **argv)
{
  Window w = 0;
  int argc = *rargc;
#define OPTION argv[0]
#define NXTOPTP ++argv, --argc > 0
#define NXTOPT(arg) if (++argv, --argc == 0) printf("%s requires an argument\n", (arg))
  while (NXTOPTP) {
    if (!strcmp(OPTION, "-id")) {
      NXTOPT("-id");
      w = 0;
      sscanf(OPTION, "0x%lx", &w);
      if (!w)
        sscanf(OPTION, "%lu", &w);
      if (!w)
        error("Invalid window id format: %s.", OPTION);
      //continue;
      break;
    }
  }
  return w;
}
