// generated by Fast Light User Interface Designer (fluid) version 1.0107

#ifndef ScoreGeneratorVstUi_h
#define ScoreGeneratorVstUi_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include "ScoreGeneratorVstFltk.hpp"
#include <FL/Fl_Tabs.H>
extern Fl_Tabs *mainTabs;
#include <FL/Fl_Group.H>
extern Fl_Group *scriptGroup;
#include <FL/Fl_Text_Editor.H>
extern Fl_Text_Editor *scriptTextEdit;
extern Fl_Group *runtimeMessagesGroup;
#include <FL/Fl_Browser.H>
extern Fl_Browser *runtimeMessagesBrowser;
extern Fl_Group *aboutGroup;
#include <FL/Fl_Text_Display.H>
extern Fl_Text_Display *aboutTextDisplay;
#include <FL/Fl_Button.H>
extern void onNew(Fl_Button*, ScoreGeneratorVstFltk*);
extern Fl_Button *newButton;
extern void onNewVersion(Fl_Button*, ScoreGeneratorVstFltk*);
extern Fl_Button *newVersionButton;
extern void onOpen(Fl_Button*, ScoreGeneratorVstFltk*);
extern Fl_Button *openButton;
extern void onSave(Fl_Button*, ScoreGeneratorVstFltk*);
extern Fl_Button *saveButton;
extern void onSaveAs(Fl_Button*, ScoreGeneratorVstFltk*);
extern Fl_Button *saveAsButton;
extern void onGenerate(Fl_Button*, ScoreGeneratorVstFltk*);
extern Fl_Button *generateButton;
Fl_Double_Window* make_window(ScoreGeneratorVstFltk *scoreGeneratorVstFltk);
#endif
