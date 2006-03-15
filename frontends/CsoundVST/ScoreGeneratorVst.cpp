/**
 * S C O R E   G E N E R A T O R   V S T
 *
 * A VST plugin for writing score generators in Python.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <Python.h>
#include "ScoreGeneratorVst.hpp"
#include "ScoreGeneratorVstFltk.hpp"
#include "System.hpp"

static char *dupstr(const char *string)
{
  if (string == 0) {
    return 0;
  }
  size_t len = std::strlen(string);
  char *copy = (char *)std::malloc(len + 1);
  std::strncpy(copy, string, len);
  copy[len] = '\0';
  return copy;
}

ScoreGeneratorVst::ScoreGeneratorVst(audioMasterCallback audioMaster) :
  AudioEffectX(audioMaster, kNumPrograms, 0),
  vstSr(0),
  vstPriorSampleBlockStart(0),
  vstCurrentSampleBlockStart(0),
  vstCurrentSampleBlockEnd(0),
  scoreGeneratorVstFltk(0)
{
  setNumInputs(kNumInputs);             // stereo in
  setNumOutputs(kNumOutputs);           // stereo out
  setUniqueID('sGsT');  // identify
  canMono();                            // makes sense to feed both inputs with the same signal
  canProcessReplacing();        // supports both accumulating and replacing output
  wantEvents();
  open();
  scoreGeneratorVstFltk = new ScoreGeneratorVstFltk(this);
  setEditor(scoreGeneratorVstFltk);
  programsAreChunks(true);
  curProgram = 0;
  bank.resize(kNumPrograms);
  for(size_t i = 0; i < bank.size(); i++)
    {
      char buffer[0x24];
      sprintf(buffer, "Program%d", (int)(i + 1));
      bank[i].name = buffer;
    }
}

ScoreGeneratorVst::~ScoreGeneratorVst()
{
  Shell::close();
}

void ScoreGeneratorVst::setProgramName(char *name)
{
  bank[curProgram].name = name;
}

void ScoreGeneratorVst::getProgramName(char *name)
{
  strcpy(name, bank[curProgram].name.c_str());
}

AEffEditor* ScoreGeneratorVst::getEditor()
{
  return editor;
}

void ScoreGeneratorVst::openView(bool doRun)
{
  editor->open(0);
//   if(doRun) {
//     run();
//   }
}

void ScoreGeneratorVst::closeView()
{
  editor->close();
}

void ScoreGeneratorVst::setProgram(long program)
{
  logf("RECEIVED ScoreGeneratorVst::setProgram(%d)...\n", program);
  if(program < kNumPrograms && program >= 0)
    {
      curProgram = program;
      setText(bank[curProgram].text);
    }
}

void ScoreGeneratorVst::suspend()
{
  log("RECEIVED ScoreGeneratorVst::suspend()...\n");
}

void ScoreGeneratorVst::resume()
{
  log("RECEIVED ScoreGeneratorVst::resume()...\n");
  wantEvents(true);
}

long ScoreGeneratorVst::processEvents(VstEvents *vstEvents)
{
  return 0;
}

void ScoreGeneratorVst::process(float **hostInput, float **hostOutput, long frames)
{
  if (alive) {
    synchronizeScore();
    sendEvents(frames);
  }
}

void ScoreGeneratorVst::processReplacing(float **hostInput, float **hostOutput, long frames)
{
  if (alive) {
    synchronizeScore();
    sendEvents(frames);
  }
}

void ScoreGeneratorVst::reset()
{
  vstSr = updateSampleRate();
  vstPriorSampleBlockStart = 0;
  vstCurrentSampleBlockStart = 0;
  vstCurrentSampleBlockEnd = 0;
  vstMidiEventsIterator = vstMidiEvents.begin();
  vstEvents.numEvents = 0;
}

void ScoreGeneratorVst::synchronizeScore()
{
  vstPriorSampleBlockStart = vstCurrentSampleBlockStart;
  VstTimeInfo *vstTimeInfo = getTimeInfo(kVstTransportPlaying);
  if ((vstTimeInfo->flags & kVstTransportPlaying) == kVstTransportPlaying)
    {
      vstCurrentSampleBlockStart = vstTimeInfo->samplePos;
    }
  if (vstPriorSampleBlockStart > vstCurrentSampleBlockStart) {
    vstMidiEventsIterator = vstMidiEvents.begin();
    vstEvents.numEvents = 0;
  }
}

void ScoreGeneratorVst::sendEvents(long frames)
{
  if (frames == 0) {
    return;
  }
  vstMidiEventsBuffer.clear();
  vstCurrentSampleBlockEnd = vstCurrentSampleBlockStart + frames;
  for(;;) {
    if (vstMidiEventsIterator == vstMidiEvents.end()) {
      return;
    }
    const VstMidiEvent &currentVstMidiEvent = *vstMidiEventsIterator;
    if (currentVstMidiEvent.deltaFrames < vstCurrentSampleBlockStart || currentVstMidiEvent.deltaFrames >= vstCurrentSampleBlockEnd) {
      return;
    }
    VstMidiEvent outputEvent = currentVstMidiEvent;
    outputEvent.deltaFrames = currentVstMidiEvent.deltaFrames - vstCurrentSampleBlockStart;
    vstMidiEventsBuffer.push_back(outputEvent);
    ++vstMidiEventsIterator;
  }
  vstEvents.events[0] = (VstEvent *)&vstMidiEventsBuffer.front();
  vstEvents.numEvents = vstMidiEventsBuffer.size();
  sendVstEventsToHost(&vstEvents);
}

bool ScoreGeneratorVst::getInputProperties(long index, VstPinProperties* properties)
{
  if(index < kNumInputs)
    {
      sprintf(properties->label, "My %ld In", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool ScoreGeneratorVst::getOutputProperties(long index, VstPinProperties* properties)
{
  if(index < kNumOutputs)
    {
      sprintf(properties->label, "My %ld Out", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool ScoreGeneratorVst::getProgramNameIndexed(long category, long index, char* text)
{
  if(index < kNumPrograms)
    {
      strcpy(text, bank[curProgram].name.c_str());
      return true;
    }
  return false;
}

bool ScoreGeneratorVst::getEffectName(char* name)
{
  strcpy(name, "ScoreGeneratorVst");
  return true;
}

bool ScoreGeneratorVst::getVendorString(char* text)
{
  strcpy(text, "Irreducible Productions");
  return true;
}

bool ScoreGeneratorVst::getProductString(char* text)
{
  strcpy(text, "ScoreGeneratorVst");
  return true;
}

long ScoreGeneratorVst::canDo(char* text)
{
  csound::System::inform("RECEIVED ScoreGeneratorVst::canDo('%s')...\n", text);
  if(strcmp(text, "receiveVstTimeInfo") == 0)
    {
      return 1;
    }
  if(strcmp(text, "receiveVstEvents") == 0)
    {
      return 0;
    }
  if(strcmp(text, "receiveVstMidiEvents") == 0)
    {
      return 0;
    }
  if(strcmp(text, "sendVstMidiEvents") == 0)
    {
      return 1;
    }
  if(strcmp(text, "plugAsChannelInsert") == 0)
    {
      return 1;
    }
  if(strcmp(text, "plugAsSend") == 0)
    {
      return 1;
    }
  if(strcmp(text, "sizeWindow") == 0)
    {
      return 1;
    }
  if(strcmp(text, "asyncProcessing") == 0)
    {
      return 0;
    }
  if(strcmp(text, "2in2out") == 0)
    {
      return 1;
    }
  return 0;
}

bool ScoreGeneratorVst::keysRequired()
{
  log("RECEIVED ScoreGeneratorVst::keysRequired...\n");
  return 1;
}

long ScoreGeneratorVst::getProgram()
{
  return curProgram;
}

bool ScoreGeneratorVst::copyProgram(long destination)
{
  logf("RECEIVED ScoreGeneratorVst::copyProgram(%d)...\n", destination);
  if(destination < kNumPrograms)
    {
      bank[destination] = bank[curProgram];
      return true;
    }
  return false;
}

long ScoreGeneratorVst::getChunk(void** data, bool isPreset)
{
  logf("BEGAN ScoreGeneratorVst::getChunk(%d)...\n", (int) isPreset);
  ((ScoreGeneratorVstFltk *)getEditor())->updateModel();
  long returnValue = 0;
  static std::string bankBuffer;
  bank[curProgram].text = getText();
  if(isPreset)
    {
      *data = (void *)bank[curProgram].text.c_str();
      returnValue = (long) strlen((char *)*data) + 1;
    }
  else
    {
      std::ostringstream stream;
      int n = bank.size();
      stream << n << "\n";
      for(std::vector<Preset>::iterator it = bank.begin(); it != bank.end(); ++it)
        {
          Preset &preset = (*it);
          stream << preset.name.c_str() << "\n";
          stream << preset.text.size() << "\n";
          for(std::string::iterator jt = preset.text.begin(); jt != preset.text.end(); ++jt)
            {
              stream.put(*jt);
            }
          stream << "\n";
        }
      bankBuffer = stream.str();
      *data = (void *)bankBuffer.c_str();
      returnValue = bankBuffer.size();
    }
  logf("ENDED ScoreGeneratorVst::getChunk, returned %d...\n", returnValue);
  return returnValue;
}

long ScoreGeneratorVst::setChunk(void* data, long byteSize, bool isPreset)
{
  logf("RECEIVED ScoreGeneratorVst::setChunk(%d, %d)...\n", byteSize, (int) isPreset);
  long returnValue = 0;
  if(isPreset)
    {
      bank[curProgram].text = dupstr((const char *)data);
      setText(bank[curProgram].text);
      returnValue = byteSize;
    }
  else
    {
#if defined(__GNUC__)
      std::string inputBuffer = (const char *)data;
      std::istringstream stream(inputBuffer);
#else
      std::istringstream stream(dupstr((const char *)data), byteSize);
#endif
      std::string buffer;
      stream >> buffer;
      stream >> std::ws;
      int n = atoi(buffer.c_str());
      bank.resize(n);
      for(int i = 0; i < n; i++)
        {
          Preset preset;
          stream >> preset.name;
          stream >> std::ws;
          stream >> buffer;
          stream >> std::ws;
          int length = atoi(buffer.c_str());
          preset.text.resize(length);
          char c;
          for(int j = 0; j < length; j++)
            {
              stream.get(c);
              preset.text[j] = c;
            }
          bank[i] = preset;
        }
      returnValue = byteSize;
    }
  setProgram(curProgram);
  editor->update();
  return returnValue;
}

std::string ScoreGeneratorVst::getText()
{
  log("BEGAN ScoreGeneratorVst::getText...");
  std::string buffer;
  buffer = getScript();
  log("ENDED ScoreGeneratorVst::getText.");
  return buffer;
}

void ScoreGeneratorVst::setText(const std::string text)
{
  setScript(text);
}

void ScoreGeneratorVst::openFile(std::string filename_)
{
  WaitCursor wait;
  load(filename_);
  setFilename(filename_);
  bank[getProgram()].text = getText();
  editor->update();
  logf("Opened file: '%s'.\n",
      filename.c_str());
  std::string drive, base, file, extension;
  csound::System::parsePathname(filename_, drive, base, file, extension);
  chdir(base.c_str());
}

static PyMethodDef scoregenMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

void ScoreGeneratorVst::open()
{
  int result = 0;
  Shell::open();
  char *argv[] = {"",""};
  PySys_SetArgv(1, argv);
  PyObject *mainModule = PyImport_ImportModule("__main__");
  result = runScript("import sys\n");
  if(result)
    {
      PyErr_Print();
    }
  result = runScript("import scoregen\n");
  if(result)
    {
      PyErr_Print();
    }
  result = runScript("score = scoregen.ScoreGenerator()\n");
  if(result)
    {
      PyErr_Print();
    }
  score = PyObject_GetAttrString(mainModule, "score");
  Py_INCREF(score);
  PyObject *pyThis = PyCObject_FromVoidPtr(this, 0);
  PyObject *pyResult = PyObject_CallMethod(score,  "setScoreGeneratorVst",  "O", pyThis);
  result = runScript("sys.stdout = sys.stderr = score\n");
  if(result)
    {
      PyErr_Print();
    }
  std::string filename_ = getFilename();
}

int ScoreGeneratorVst::runScript(std::string script_)
{
  log("BEGAN ScoreGeneratorVst::runScript()...\n");
  int result = 0;
  try
    {
      char *script__ = const_cast<char *>(script_.c_str());
      log("==============================================================================================================\n");
      result = PyRun_SimpleString(script__);
      if(result)
        {
          PyErr_Print();
        }
    }
  catch(...)
    {
      log("Unidentified exception in ScoreGeneratorVst::runScript().\n");
    }
  log("==============================================================================================================\n");
  logf("PyRun_SimpleString returned %d.\n", result);
  log("ENDED ScoreGeneratorVst::runScript().\n");
  return result;
}

int ScoreGeneratorVst::generate()
{
  clearEvents();
  Shell::run();
  sortEvents();
  alive = true;
}

void ScoreGeneratorVst::clearEvents()
{
  alive = false;
  vstEvents.numEvents = 0;
  vstMidiEvents.clear();
  reset();
}

void ScoreGeneratorVst::sortEvents()
{
  std::sort(vstMidiEvents.begin(), vstMidiEvents.end(), MidiSort());
}

void ScoreGeneratorVst::event(double start, double duration, double status, double channel, double data1, double data2)
{
  midiopcode = char(status) & char(0xf0);
  midichannel = char(channel) & char(0xf);
  midistatus = midiopcode | midichannel;
  char detune = 0;
  // Round down.
  if (midiopcode == 0x90) {
    midikey = char(data1 + 0.5) & char(0xf0);
    detune = char((data1 - double(midikey)) / 100.);
  }
  midivelocity = char(data2) & char(0xf0);
  VstMidiEvent noteon;
  vstMidiEvent.type      = kVstMidiType;
  noteon.byteSize        = 24;
  noteon.deltaFrames     = long(vstSr * start);
  noteon.flags           = 0;
  noteon.noteLength      = long(vstSr * duration);
  noteon.noteOffset      = 0;
  noteon.midiData[0]     = midistatus;
  noteon.midiData[1]     = midikey;
  noteon.midiData[2]     = midivelocity;
  noteon.midiData[3]     = 0;
  noteon.detune          = detune;
  vstMidiEvents.push_back(noteon);
//   if (duration > 0) {
//     double stop = start + duration;
//     VstMidiEvent noteoff = noteon;
//     noteoff.deltaFrames = long(vstSr * stop);
//     noteoff.midiData[0] = midistatus - char(0x10);
//     noteoff.midiData[2] = char(0);
//     vstMidiEvents.push_back(noteoff);
//   }
  return vstMidiEvents.size();
}

void ScoreGeneratorVst::log(char *message)
{
  if (scoreGeneratorVstFltk) {
    scoreGeneratorVstFltk->log(message);
  }
}

void ScoreGeneratorVst::logf(char *format,...)
{
  char buffer[0x100];
  va_list marker;
  va_start(marker, format);
  vsprintf(buffer, format, marker);
  log(buffer);
  va_end(marker);
}

