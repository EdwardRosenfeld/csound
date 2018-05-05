/*
 * C S O U N D
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
 */


// Setup a single global AudioContext object
var CSOUND_AUDIO_CONTEXT = CSOUND_AUDIO_CONTEXT || 
    (function() {

        try {
            var AudioContext = window.AudioContext || window.webkitAudioContext;
            return new AudioContext();  
        }
        catch(error) {

            console.log('Web Audio API is not supported in this browser');
        }
        return null;
    }());


// Global singleton variables
var AudioWorkletGlobalScope = AudioWorkletGlobalScope || {};
var CSOUND;

/** This E6 class is used to setup scripts and
    allow the creation of new CsoundScriptProcessorNode objects
    *  @hideconstructor
    */
class CsoundScriptProcessorNodeFactory {

    // Utility function to load a script and set callback
    static loadScript(src, callback) {
        var script = document.createElement('script');
        script.src = src;
        script.onload = callback;
        document.head.appendChild(script);
    }

    /** 
     * This static method is used to asynchronously setup scripts for 
     *  ScriptProcessorNode Csound
     *
     * @param {string} script_base A string containing the base path to scripts
     */
    static importScripts(script_base='./') {
        return new Promise((resolve) => {
            CsoundScriptProcessorNodeFactory.loadScript(script_base + 'libcsound.js', () => {
                AudioWorkletGlobalScope.WAM = {}
                let WAM = AudioWorkletGlobalScope.WAM;

                WAM["ENVIRONMENT"] = "WEB";
                WAM["print"] = (t) => console.log(t);
                WAM["printErr"] = (t) => console.log(t);
                WAM["locateFile"] = (f) => script_base + f;

                AudioWorkletGlobalScope.libcsound(WAM).then(() => {

                    // Cache cwrap functions into CSOUND global object 
                    CSOUND = {
                        new: WAM.cwrap('CsoundObj_new', ['number'], null),
                        compileCSD: WAM.cwrap('CsoundObj_compileCSD', ['number'], ['number', 'string']),
                        evaluateCode: WAM.cwrap('CsoundObj_evaluateCode', ['number'], ['number', 'string']),
                        readScore: WAM.cwrap('CsoundObj_readScore', ['number'], ['number', 'string']),
                        reset: WAM.cwrap('CsoundObj_reset', null, ['number']),
                        getOutputBuffer: WAM.cwrap('CsoundObj_getOutputBuffer', ['number'], ['number']),
                        getInputBuffer: WAM.cwrap('CsoundObj_getInputBuffer', ['number'], ['number']),
                        getControlChannel: WAM.cwrap('CsoundObj_getControlChannel', ['number'], ['number', 'string']),
                        setControlChannel: WAM.cwrap('CsoundObj_setControlChannel', null, ['number', 'string', 'number']),
                        setStringChannel: WAM.cwrap('CsoundObj_setStringChannel', null, ['number', 'string', 'string']),
                        getKsmps: WAM.cwrap('CsoundObj_getKsmps', ['number'], ['number']),
                        performKsmps: WAM.cwrap('CsoundObj_performKsmps', ['number'], ['number']),
                        render: WAM.cwrap('CsoundObj_render', null, ['number']),
                        getInputChannelCount: WAM.cwrap('CsoundObj_getInputChannelCount', ['number'], ['number']),
                        getOutputChannelCount: WAM.cwrap('CsoundObj_getOutputChannelCount', ['number'], ['number']),
                        getTableLength: WAM.cwrap('CsoundObj_getTableLength', ['number'], ['number', 'number']),
                        getTable: WAM.cwrap('CsoundObj_getTable', ['number'], ['number', 'number']),
                        getZerodBFS: WAM.cwrap('CsoundObj_getZerodBFS', ['number'], ['number']),
                        setMidiCallbacks: WAM.cwrap('CsoundObj_setMidiCallbacks', null, ['number']),
                        pushMidiMessage: WAM.cwrap('CsoundObj_pushMidiMessage', null, ['number', 'number', 'number', 'number']),
                        setOutputChannelCallback: WAM.cwrap('CsoundObj_setOutputChannelCallback', null, ['number', 'number']),
                        compileOrc: WAM.cwrap('CsoundObj_compileOrc', 'number', ['number', 'string']),
                        setOption: WAM.cwrap('CsoundObj_setOption', null, ['number', 'string']),
                        prepareRT: WAM.cwrap('CsoundObj_prepareRT', null, ['number']),
                        getScoreTime: WAM.cwrap('CsoundObj_getScoreTime', null, ['number']),
                        setTable: WAM.cwrap('CsoundObj_setTable', null, ['number', 'number', 'number', 'number']),
                        destroy: WAM.cwrap('CsoundObj_destroy', null, ['number'])
                    }

                    resolve();
                }) 
            }) 
        });
    }

    /** 
     * This static method creates a new CsoundScriptProcessorNode. 
     *  @param {number} inputChannelCount Number of input channels
     *  @param {number} outputChannelCount Number of output channels
     *  @returns {object} A new CsoundScriptProcessorNode
     */
    static createNode(inputChannelCount=1, outputChannelCount=2) {
        /**  @classdesc A CsoundScriptProcessorNode containing a Csound engine
         *   @class CsoundScriptProcessorNode
         *   @mixes CsoundMixin
         *   @hideconstructor
         */
        var spn = CSOUND_AUDIO_CONTEXT.createScriptProcessor(0, inputChannelCount, outputChannelCount);
        spn.inputCount = inputChannelCount;
        spn.outputCount = outputChannelCount;

        let cs = CSOUND.new();
        CSOUND.setMidiCallbacks(cs);
        CSOUND.setOption(cs, "-odac");
        CSOUND.setOption(cs, "-iadc");
        CSOUND.setOption(cs, "-M0");
        CSOUND.setOption(cs, "-+rtaudio=null");
        CSOUND.setOption(cs, "-+rtmidi=null");
        CSOUND.prepareRT(cs);
        var sampleRate = CSOUND_AUDIO_CONTEXT.sampleRate;
        CSOUND.setOption(cs, "--sample-rate="+sampleRate);
        CSOUND.setOption(cs, "--nchnls=" + this.nchnls);
        CSOUND.setOption(cs, "--nchnls_i=" + this.nchnls_i); 

        /**  Provides methods to manipulate an ScriptProcessorNode
         *
         *   @mixin CsoundMixin
         */
        let CsoundMixin = {
            csound: cs,
            compiled: false,
            msgCallback: (t) => console.log(t),
            csoundRunning: false,
            csoundOutputBuffer: null,
            csoundInputBuffer: null,
            zerodBFS: 1.0,
            offset: 32,
            ksmps: 32,
            running: false,
            started: false,
            cnt: 0,
            res: 0,
            nchnls_i: inputChannelCount, 
            nchnls: outputChannelCount,         

            /** 
             *
             *  Writes data to a file in the WASM filesystem for
             *  use with csound.
             *
             * @param {string} filePath A string containing the path to write to.
             * @param {blob}   blobData The data to write to file.
             * @memberof CsoundMixin
             */ 
            writeToFS(filePath, blobData) {
                let FS = WAM["FS"];
                let stream = FS.open(filePath, 'w+');
                let buf = new Uint8Array(blobData)
                FS.write(stream, buf, 0, buf.length, 0);
                FS.close(stream);
            },
            
            /** Compiles a CSD, which may be given as a filename in the
             *  WASM filesystem or a string containing the code
             *
             * @param {string} csd A string containing the CSD filename or the CSD code.
             * @memberof CsoundMixin
             */ 
            compileCSD(csd) {
                CSOUND.prepareRT(this.csound);
                CSOUND.compileCSD(this.csound, filePath);
                this.compiled = true;
            },

            /** Compiles Csound orchestra code.
             *
             * @param {string} orcString A string containing the orchestra code.
             * @memberof CsoundMixin
             */  
            compileOrc(orcString) {
                CSOUND.prepareRT(this.csound);
                CSOUND.compileOrc(this.csound, orcString);
                this.compiled = true;
            },

            /** Sets a Csound engine option (flag)
             *  
             *
             * @param {string} option The Csound engine option to set. This should
             * not contain any whitespace.
             * @memberof CsoundMixin
             */  
            setOption(option) {
                CSOUND.setOption(this.csound, option);
            },

            /** Renders a CSD, which may be given as a filename in the
             *  WASM filesystem or a string containing the code. This is used for
             *  disk rendering only.
             * @param {string} csd A string containing the CSD filename or the CSD code.
             * @memberof CsoundMixin
             */ 
            render(filePath) {
                CSOUND.compileCSD(this.csound, filePath);
                CSOUND.render(this.csound);
                this.compiled = false;
            },

            /** Evaluates Csound orchestra code.
             *
             * @param {string} codeString A string containing the orchestra code.
             * @memberof CsoundMixin
             */  
            evaluateCode(codeString) {
                CSOUND.evaluateCode(this.csound, codeString);
            },
            /** Reads a numeric score string.
             *
             * @param {string} scoreString A string containing a numeric score.
             * @memberof CsoundMixin
             */   
            readScore(scoreString) {
                CSOUND.readScore(this.csound, scoreString);
            },
            
            /** Sets the value of a control channel in the software bus
             *
             * @param {string} channelName A string containing the channel name.
             * @param {number} value The value to be set.
             * @memberof CsoundMixin
             */ 
            setControlChannel(channelName, value) {
                CSOUND.setControlChannel(this.csound, channelName, value);
            },
            
            /** Sets the value of a string channel in the software bus
             *
             * @param {string} channelName A string containing the channel name.
             * @param {string} stringValue The string to be set.
             * @memberof CsoundMixin
             */ 
            setStringChannel(channelName, value) {
                CSOUND.setStringChannel(this.csound, channelName, value); 
            },
            
            /** Starts processing in this node
             *  @memberof CsoundMixin
             */ 
            start() {
                if(this.started == false) {
                    let ksmps = CSOUND.getKsmps(this.csound);
                    this.ksmps = ksmps;
                    this.cnt = ksmps;

                    let outputPointer = CSOUND.getOutputBuffer(this.csound);
                    this.csoundOutputBuffer = new Float32Array(WAM.HEAP8.buffer, outputPointer, ksmps * outputChannelCount);
                    let inputPointer = CSOUND.getInputBuffer(this.csound);
                    this.csoundInputBuffer = new Float32Array(WAM.HEAP8.buffer, inputPointer, ksmps * inputChannelCount);
                    this.zerodBFS = CSOUND.getZerodBFS(this.csound);
                    this.started = true;
                }
                this.running = true;

            },

            /** Resets the Csound engine.
             *  @memberof CsoundMixin
             */ 
            reset() {
                CSOUND.reset(this.csound);
                this.compiled = false;
            },

            destroy() {
                CSOUND.destroy(this.csound);
            },

            /** Starts performance, same as start()
             * @memberof CsoundMixin
             */ 
            play() {
                CSOUND.start(this.csound); 
            },

            /** Stops (pauses) performance
             *   @memberof CsoundMixin
             */      
            stop() {
                this.running = false;
            },

            /** Sets a callback to process Csound console messages.
             *
             * @param {function} msgCallback A callback to process messages 
             * with signature function(message), where message is a string
             * from Csound.
             * @memberof CsoundMixin
             */           
            setMessageCallback(msgCallback) {
                this.msgCallBack = msgCallback;
            },
            
            /** Sends a MIDI channel message to Csound
             *
             * @param {number} byte1 MIDI status byte
             * @param {number} byte2 MIDI data byte 1
             * @param {number} byte1 MIDI data byte 2
             *
             * @memberof CsoundMixin
             */  
            midiMessage(byte1, byte2, byte3) {
                CSOUND.pushMidiMessage(this.csound, byte1, byte2, byte3);
            },

            onaudioprocess(e) {
                if (this.csoundOutputBuffer == null ||
                    this.running == false) {
                    return;
                }

                let input = e.inputBuffer;
                let output = e.outputBuffer;

                let bufferLen = output.getChannelData(0).length;

                let csOut = this.csoundOutputBuffer;
                let csIn = this.csoundInputBuffer;
                let ksmps = this.ksmps;
                let zerodBFS = this.zerodBFS;

                let cnt = this.cnt;
                let nchnls = this.nchnls;
                let nchnls_i = this.nchnls_i;  
                let res = this.res;

                for (let i = 0; i < bufferLen; i++, cnt++) {
                    if(cnt >= ksmps && res == 0) {
                        // if we need more samples from Csound
                        res = CSOUND.performKsmps(this.csound);
                        cnt = 0;
                    }

                    for (let channel = 0; channel < input.numberOfChannels; channel++) {
                        let inputChannel = input.getChannelData(channel);
                        csIn[cnt*nchnls_i + channel] =  inputChannel[i] * zerodBFS;
                    }
                    for (let channel = 0; channel < output.numberOfChannels; channel++) {
                        let outputChannel = output.getChannelData(channel);
                        if(res == 0)
                            outputChannel[i] = csOut[cnt*nchnls + channel] / zerodBFS;
                        else
                            outputChannel[i] = 0;
                    } 
                }

                this.cnt = cnt;
                this.res = res;
            }
        }

        let WAM = AudioWorkletGlobalScope.WAM;
        WAM["print"] = (t) => spn.msgCallback(t);
        WAM["printErr"] = (t) => spn.msgCallback(t);

        return Object.assign(spn,CsoundMixin);
    }
}
