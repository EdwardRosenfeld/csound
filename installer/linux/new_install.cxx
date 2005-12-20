#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int do_install = 0;
int end_alert = 0;
int do_libinstall = 0;
int lib_exit = 0;

char type[256];
char bin[256];
char opc[256];
char doh[256];
char lib[256];
char envy[256];

# include "installer.cxx"

#if defined(linux)
# define LIBEXT "so"
#elif defined(__MACH__)
# define LIBEXT "dynlib"
#elif
#define LIBEXT "dll"
#endif

void set_system(Fl_Check_Button*, void*)
{
    bindir->value(bin);
    opcdir->value(opc);
    doc->value(doh);
    libdir->value(lib);
}

Fl_Double_Window* err;

void do_alert(char *msg)
{
      err_text->value(msg);
      end_alert = 0;
      err->show();
      while (end_alert==0) Fl::wait();
      err->hide();
}

/* Unsure that dir exists with all directories on the way. */
void check_exists(const char *dir)
{
    char test[80];
    char *p;
    int ans;
    struct stat buf;
    strcpy(test,dir);
    //    printf("Checking %s\n", dir);
    p = test;
    while ((p = strchr(p+1, '/')) != NULL) {
      *p = '\0';
      if (test[0]=='\0') break; // Should not happen
      //      printf("..Checking %s\n", test);
      ans = stat(test, &buf);
      if (ans!=0 || !S_ISDIR(buf.st_mode)) {
        if (ans!=0) {
          //          printf("Directory %s does not exist; creating...\n", test);
          mkdir(test, 0766);
        }
        else {
          do_alert("Trouble with file; stopping");
          exit(1);
        }
      }
      else {
        //        printf("Directory %s OK\n", test);
      }
      *p = '/';
    }
    return;
}

void wrap(char *dest, char *src, const char *file, const char *opcd)
{
    /* Need to setup OPCODEDIR or OPCODEDIR64 */
    /* This differs according to which shell is being used, so for
       bash/sh add to .profile "OPCODEDIRxx=yyy; export OPCODEDIRxx"
       csh/tcsh add to .cshrc "setenv OPCODEDIRxx yyyy"
    */
    char buff[120];
    char binlink[256];
    char oplink[256];
    FILE *rc;
    //printf("wrap: dest=%s src=%s file=%s opcd=%s\n", dest, src, file, opcd);
    // Make full address
    if (bindir->value()[0]!='/') {
      char bb[200];
      getcwd(bb, 200);
      sprintf(binlink, "%s/%s", bb, src);
    }
    else
      strcpy(binlink, file);
    //printf("    : binlink=%s\n", binlink);
    if (opcdir->value()[0]!='/') {
      char bb[200];
      getcwd(bb, 200);
      sprintf(oplink, "%s/%s", bb, opcd);
    }
    else
      strcpy(oplink, opcd);
    //printf("    : oplink=%s\n", oplink);
    sprintf(buff, "%s/%s", dest, file);
    rc = fopen(buff, "w");
    fprintf(rc, "#!/bin/sh\n%s=%s\n%s/%s $0\n",
            envy, oplink, binlink, file);
    fclose(rc);
    chmod(buff,S_IEXEC|S_IREAD|S_IWRITE|S_IXGRP|S_IXOTH);
}


int main(void)
{
    FILE *defs = fopen("def.ins", "r");
    Fl_Double_Window* www;
    char *p;
    if (defs==0) {
      err = make_alert();
      do_alert("Definitions file is missing");
      exit(1);
    }
    fgets(type,256,defs);
    p = strchr(type,'\n');
    if (p!=type) *p = '\0';
    fgets(bin,256,defs);
    p = strchr(bin,'\n');
    if (p!=bin) *p = '\0';
    fgets(opc,256,defs);
    p = strchr(opc,'\n');
    if (p!=opc) *p = '\0';
    fgets(doh,256,defs);
    p = strchr(doh,'\n');
    if (p!=doh) *p = '\0';
    fgets(lib,256,defs);
    p = strchr(lib,'\n');
    if (p!=lib) *p = '\0';
    fgets(envy,256,defs);
    p = strchr(envy,'\n');
    if (p!=envy) *p = '\0';
    www = make_window(type);
    doBin->value(1); doOpc->value(1);
    www->show();
    err = make_alert();
 again:
    while (do_install==0) Fl::wait(1.0);
    // Check that install is correct
    if (doBin->value() && strlen(bindir->value())==0) {
      do_alert("No binary directory");
      goto again;
    }
    if (doOpc->value() && strlen(opcdir->value())==0) {
      do_alert("No opcode directory");
      goto again;
    }
    //Copy binaries
    if (doBin->value()) {
      struct dirent **namelist;
      char b[256];
      char c[256];
      int n = scandir("./bin", &namelist, NULL, alphasort);
      int i;
      float pr = 0;
      progress->label("binaries");
      strcpy(b, bindir->value());
      if (b[strlen(b)-1]!='/')
        strcat(b, "/");
      strcpy(c, b);
      strcat(c, "bin/");
      check_exists(c);
      progress->minimum(0.0f); progress->maximum((float)(n+n));
      progress->value(0.0f);
      Fl::wait(0.1);
      for (i=0; i<n; i++)
        if ((namelist[i]->d_name)[0]!='.') {
          char buff[256];
          sprintf(buff,"cp -pv ./bin/%s %s >/dev/null", namelist[i]->d_name,c);
          //printf("**** %s\n", buff);
          system(buff);
          progress->value(pr+= 1.0f);
          Fl::wait(0.1);
          if (strlen(opcdir->value())!=0)
            wrap(b, c, namelist[i]->d_name, opcdir->value());
          progress->value(pr+1.0f);
          Fl::wait(0.1);
        }
        else progress->value(pr+= 2.0f);
    }
    //Copy opcodes
    if (doOpc->value()) {
      struct dirent **namelist;
      char b[256];
      int n = scandir("./opc", &namelist, NULL, alphasort);
      int i;
      progress->label("opcodes");
      strcpy(b, opcdir->value());
      if (b[strlen(b)-1]!='/')
        strcat(b, "/");
      check_exists(b);
      progress->minimum(0.0f); progress->maximum((float)n);
      progress->value(0.0f);
      Fl::wait(0.1);
      for (i=0; i<n; i++)
        if ((namelist[i]->d_name)[0]!='.') {
          char buff[256];
          sprintf(buff,"cp -pv ./opc/%s %s>/dev/null", namelist[i]->d_name,b);
          system(buff);
          progress->value((float)(i+1));
          Fl::wait(0.1);
        }
        else progress->value((float)(i+1));
    }
    if (doDoc->value() && strlen(doc->value())!=0) {
      struct dirent **namelist;
      char b[256];
      int n = scandir("./html", &namelist, NULL, alphasort);
      int i;
      progress->label("documentation");
      strcpy(b, doc->value());
      check_exists(b);
      if (b[strlen(b)-1]!='/')
        strcat(b, "/");
      check_exists(b);
      check_exists(doc->value());
      progress->minimum(0.0f); progress->maximum((float)n);
      progress->value(0.0f);
      Fl::wait(0.1);
      for (i=0; i<n; i++)
        if ((namelist[i]->d_name)[0]!='.') {
          char buff[256];
          sprintf(buff,"cp -pv ./html/%s %s>/dev/null", namelist[i]->d_name,b);
          system(buff);
          progress->value((float)(i+1));
          Fl::wait(0.1);
        }
        else progress->value((float)(i+1));
    }
    if (doLib->value() && strlen(libdir->value())!=0) {
      char b[256];
      Fl_Double_Window *ll = make_libraries();
      strcpy(b, libdir->value());
      // really ought to pre-mark acording to availability
      lib_exit = 0;
      ll->show();
      while (lib_exit==0) Fl::wait();
      progress->label("libraries");
      progress->minimum(0.0f); progress->maximum(7.0);
      progress->value(0.0f);
      if (do_libinstall) {
        char buff[256];
        char name[256];
        char *p;
        float n = 0.0f;
        fgets(name,256,defs);
        p = strchr(name,'\n');
        if (p!=name) *p = '\0';
        check_exists(b);
        sprintf(buff,"cp -pv ./lib/%s %s>/dev/null", name, b);
        system(buff);
        progress->value(n+= 1.0f);
        if (do_asound->value()) {
          sprintf(buff,"cp -pv ./lib/libasound." LIBEXT ".2 %s>/dev/null",  b);
          system(buff);
          sprintf(buff,"ln -s %s/libasound." LIBEXT ".2 %s/libasound." LIBEXT,b,b);
          system(buff);
          progress->value(n+= 1.0f);
        }
        if (do_fluidsynth->value()) {
          sprintf(buff,"cp -pv ./lib/libfluidsynth." LIBEXT ".1 %s>/dev/null", b);
          system(buff);
          sprintf(buff,"ln -s %s/libfluidsynth." LIBEXT
                  ".1 %s/libfluidsynth." LIBEXT,b,b);
          system(buff);
          progress->value(n+= 1.0f);
        }
        if (do_jack->value()) {
          sprintf(buff,"cp -pv ./lib/libjack." LIBEXT ".0 %s>/dev/null", b);
          system(buff);
          sprintf(buff,"ln -s %s/libjacl.0 %s/libjack." LIBEXT "",b,b);
          system(buff);
          progress->value(n+= 1.0f);
        }
        if (do_lo->value()) {
          sprintf(buff,"cp -pv ./lib/liblo." LIBEXT ".0 %s>/dev/null", b);
          system(buff);
          sprintf(buff,"ln -s %s/liblo." LIBEXT ".0 %s/liblo." LIBEXT "",b,b);
          system(buff);
          progress->value(n+= 1.0f);
        }
        if (do_portaudio->value()) {
          sprintf(buff,"cp -pv ./lib/libportaudio." LIBEXT " %s>/dev/null", b);
          system(buff);
          progress->value(n+= 1.0f);
        }
        if (do_sndfile->value()) {
          sprintf(buff,"cp -pv ./lib/libsndfile." LIBEXT ".1 %s>/dev/null", b);
          system(buff);
          sprintf(buff,"ln -s %s/libsndfile." LIBEXT
                  ".1 %s/libsndfile." LIBEXT,b,b);
          system(buff);
          progress->value(n+= 1.0f);
        }
      }
    }
    err->color(FL_GRAY);
    do_alert("Installation finished");
}
