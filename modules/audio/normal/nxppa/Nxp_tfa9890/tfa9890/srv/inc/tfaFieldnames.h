/*
 * this file was generated from the register list:
 *   (cd $(TOOLS)/TfaRegdefs;$(TOOLS)/TfaRegdefs.py -ds)
 */
#ifndef TFAFIELDNAMES_H_
#define TFAFIELDNAMES_H_

typedef enum nxpTfaBfEnumList {
    bfVDDS  = 0x0000,    /* POR                                                */
    bfPLLS  = 0x0010,    /* PLL                                                */
    bfOTDS  = 0x0020,    /* OTP                                                */
    bfOVDS  = 0x0030,    /* OVP                                                */
    bfUVDS  = 0x0040,    /* UVP                                                */
    bfOCDS  = 0x0050,    /* OCP                                                */
    bfCLKS  = 0x0060,    /* Clocks                                             */
    bfCLIPS = 0x0070,    /* CLIP                                               */
    bfMTPB  = 0x0080,    /* MTP                                                */
    bfDCCS  = 0x0090,    /* BOOST                                              */
    bfSPKS  = 0x00a0,    /* Speaker                                            */
    bfACS   = 0x00b0,    /* cold start flag                                    */
    bfSWS   = 0x00c0,    /* flag engage                                        */
    bfWDS   = 0x00d0,    /* flag watchdog reset                                */
    bfAMPS  = 0x00e0,    /* amplifier is enabled by manager                    */
    bfAREFS = 0x00f0,    /* references are enabled by manager                  */
    bfBATS  = 0x0109,    /* Battery voltage readout; 0[V]..5.5[V]              */
    bfTEMPS = 0x0208,    /* Temperature readout                                */
    bfREV   = 0x0307,    /*                                                    */
    bfI2SF  = 0x0402,    /* I2SFormat data 1 input:                            */
    bfCHS12 = 0x0431,    /* ChannelSelection data1 input  (In CoolFlux)        */
    bfCHS3  = 0x0450,    /* ChannelSelection data 2 input (coolflux input, the DCDC converter gets the other signal) */
    bfCHSA  = 0x0461,    /* Input selection for amplifier                      */
    bfI2SDOC= 0x0481,    /* selection data out                                 */
    bfDISP  = 0x04a0,    /* idp protection                                     */
    bfI2SDOE= 0x04b0,    /* Enable data output                                 */
    bfI2SSR = 0x04c3,    /* sample rate setting                                */
    bfBSSCR = 0x0501,    /* ProtectionAttackTime                               */
    bfBSST  = 0x0523,    /* ProtectionThreshold                                */
    bfBSSRL = 0x0561,    /* ProtectionMaximumReduction                         */
    bfBSSRR = 0x0582,    /* Protection Release Timer                           */
    bfBSSHY = 0x05b1,    /* ProtectionHysterese                                */
    bfBSSR  = 0x05d0,    /* reset clipper                                      */
    bfBSAV  = 0x05e0,    /* battery voltage for I2C read out only              */
    bfBSSBY = 0x05f0,    /* bypass clipper battery protection                  */
    bfDPSA  = 0x0600,    /* Enable dynamic powerstage activation               */
    bfAMPSL = 0x0613,    /* control slope                                      */
    bfCFSM  = 0x0650,    /* Soft mute in CoolFlux                              */
    bfBSSS  = 0x0670,    /* batsensesteepness                                  */
    bfVOL   = 0x0687,    /* volume control (in CoolFlux)                       */
    bfDCVO  = 0x0702,    /* Boost voltage                                      */
    bfDCMCC = 0x0732,    /* Max boost coil current                             */
    bfDCIE  = 0x07a0,    /* Intelligent boost mode                             */
    bfDCSR  = 0x07b0,    /* Soft RampUp/Down mode for DCDC controller          */
    bfTROS  = 0x0800,    /* select external temperature also the ext_temp will be put on the temp read out  */
    bfEXTTS = 0x0818,    /* external temperature setting to be given by host   */
    bfPWDN  = 0x0900,    /* ON/OFF                                             */
    bfI2CR  = 0x0910,    /* I2CReset                                           */
    bfCFE   = 0x0920,    /* EnableCoolFlux                                     */
    bfAMPE  = 0x0930,    /* EnableAmplifier                                    */
    bfDCA   = 0x0940,    /* EnableBoost                                        */
    bfSBSL  = 0x0950,    /* Coolflux configured                                */
    bfAMPC  = 0x0960,    /* Selection on how AmplifierEnabling                 */
    bfDCDIS = 0x0970,    /* DCDC switch off                                    */
    bfPSDR  = 0x0980,    /* Iddq test amplifier                                */
    bfDCCV  = 0x0991,    /* Coil Value                                         */
    bfCCFD  = 0x09b1,    /* Selection CoolFluxClock                            */
    bfISEL  = 0x09d0,    /* selection input 1 or 2                             */
    bfIPLL  = 0x09e0,    /* selection input PLL for lock                       */
    bfDOLS  = 0x0a02,    /* Output selection dataout left channel              */
    bfDORS  = 0x0a32,    /* Output selection dataout right channel             */
    bfSPKL  = 0x0a62,    /* Selection speaker induction                        */
    bfSPKR  = 0x0a91,    /* Selection speaker impedance                        */
    bfDCFG  = 0x0ab3,    /* DCDC speaker current compensation gain             */
    bfMTPK  = 0x0b07,    /* 5Ah, 90d To access hidden registers (=Default for engineering) */
    bfVDDD  = 0x0f00,    /* mask flag_por for interupt generation              */
    bfOTDD  = 0x0f10,    /* mask flag_otpok for interupt generation            */
    bfOVDD  = 0x0f20,    /* mask flag_ovpok for interupt generation            */
    bfUVDD  = 0x0f30,    /* mask flag_uvpok for interupt generation            */
    bfOCDD  = 0x0f40,    /* mask flag_ocp_alarm for interupt generation        */
    bfCLKD  = 0x0f50,    /* mask flag_clocks_stable for interupt generation    */
    bfDCCD  = 0x0f60,    /* mask flag_pwrokbst for interupt generation         */
    bfSPKD  = 0x0f70,    /* mask flag_cf_speakererror for interupt generation  */
    bfWDD   = 0x0f80,    /* mask flag_watchdog_reset for interupt generation   */
    bfINT   = 0x0fe0,    /* enabling interrupt                                 */
    bfINTP  = 0x0ff0,    /* Setting polarity interupt                          */
    bfCIMTP = 0x62b0,    /* start copying all the data from i2cregs_mtp to mtp [Key 2 protected] */
    bfRST   = 0x7000,    /* Reset CoolFlux DSP                                 */
    bfDMEM  = 0x7011,    /* Target memory for access                           */
    bfAIF   = 0x7030,    /* Autoincrement-flag for memory-address              */
    bfCFINT = 0x7040,    /* Interrupt CoolFlux DSP                             */
    bfREQ   = 0x7087,    /* request for access (8 channels)                    */
    bfMADD  = 0x710f,    /* memory-address to be accessed                      */
    bfMEMA  = 0x720f,    /* activate memory access (24- or 32-bits data is written/read to/from memory */
    bfERR   = 0x7307,    /* cf error Flags                                     */
    bfACK   = 0x7387,    /* acknowledge of requests (8 channels")"             */
    bfMTPOTC= 0x8000,    /* Calibration schedule (key2 protected)              */
    bfMTPEX = 0x8010,    /* (key2 protected)                                   */
} nxpTfaBfEnumList_t;
typedef struct TfaBfName {
   unsigned short bfEnum;
   char  *bfName;
} tfaBfName_t;

#define TFA_NAMETABLE static tfaBfName_t TfaBfNames[]= {\
   { 0x00, "VDDS"},    /* POR                                               , */\
   { 0x10, "PLLS"},    /* PLL                                               , */\
   { 0x20, "OTDS"},    /* OTP                                               , */\
   { 0x30, "OVDS"},    /* OVP                                               , */\
   { 0x40, "UVDS"},    /* UVP                                               , */\
   { 0x50, "OCDS"},    /* OCP                                               , */\
   { 0x60, "CLKS"},    /* Clocks                                            , */\
   { 0x70, "CLIPS"},    /* CLIP                                              , */\
   { 0x80, "MTPB"},    /* MTP                                               , */\
   { 0x90, "DCCS"},    /* BOOST                                             , */\
   { 0xa0, "SPKS"},    /* Speaker                                           , */\
   { 0xb0, "ACS"},    /* cold start flag                                   , */\
   { 0xc0, "SWS"},    /* flag engage                                       , */\
   { 0xd0, "WDS"},    /* flag watchdog reset                               , */\
   { 0xe0, "AMPS"},    /* amplifier is enabled by manager                   , */\
   { 0xf0, "AREFS"},    /* references are enabled by manager                 , */\
   { 0x109, "BATS"},    /* Battery voltage readout; 0[V]..5.5[V]             , */\
   { 0x208, "TEMPS"},    /* Temperature readout                               , */\
   { 0x307, "REV"},    /*                                                   , */\
   { 0x402, "I2SF"},    /* I2SFormat data 1 input:                           , */\
   { 0x431, "CHS12"},    /* ChannelSelection data1 input  (In CoolFlux)       , */\
   { 0x450, "CHS3"},    /* ChannelSelection data 2 input (coolflux input, the DCDC converter gets the other signal), */\
   { 0x461, "CHSA"},    /* Input selection for amplifier                     , */\
   { 0x481, "I2SDOC"},    /* selection data out                                , */\
   { 0x4a0, "DISP"},    /* idp protection                                    , */\
   { 0x4b0, "I2SDOE"},    /* Enable data output                                , */\
   { 0x4c3, "I2SSR"},    /* sample rate setting                               , */\
   { 0x501, "BSSCR"},    /* ProtectionAttackTime                              , */\
   { 0x523, "BSST"},    /* ProtectionThreshold                               , */\
   { 0x561, "BSSRL"},    /* ProtectionMaximumReduction                        , */\
   { 0x582, "BSSRR"},    /* Protection Release Timer                          , */\
   { 0x5b1, "BSSHY"},    /* ProtectionHysterese                               , */\
   { 0x5d0, "BSSR"},    /* reset clipper                                     , */\
   { 0x5e0, "BSAV"},    /* battery voltage for I2C read out only             , */\
   { 0x5f0, "BSSBY"},    /* bypass clipper battery protection                 , */\
   { 0x600, "DPSA"},    /* Enable dynamic powerstage activation              , */\
   { 0x613, "AMPSL"},    /* control slope                                     , */\
   { 0x650, "CFSM"},    /* Soft mute in CoolFlux                             , */\
   { 0x670, "BSSS"},    /* batsensesteepness                                 , */\
   { 0x687, "VOL"},    /* volume control (in CoolFlux)                      , */\
   { 0x702, "DCVO"},    /* Boost voltage                                     , */\
   { 0x732, "DCMCC"},    /* Max boost coil current                            , */\
   { 0x7a0, "DCIE"},    /* Intelligent boost mode                            , */\
   { 0x7b0, "DCSR"},    /* Soft RampUp/Down mode for DCDC controller         , */\
   { 0x800, "TROS"},    /* select external temperature also the ext_temp will be put on the temp read out , */\
   { 0x818, "EXTTS"},    /* external temperature setting to be given by host  , */\
   { 0x900, "PWDN"},    /* ON/OFF                                            , */\
   { 0x910, "I2CR"},    /* I2CReset                                          , */\
   { 0x920, "CFE"},    /* EnableCoolFlux                                    , */\
   { 0x930, "AMPE"},    /* EnableAmplifier                                   , */\
   { 0x940, "DCA"},    /* EnableBoost                                       , */\
   { 0x950, "SBSL"},    /* Coolflux configured                               , */\
   { 0x960, "AMPC"},    /* Selection on how AmplifierEnabling                , */\
   { 0x970, "DCDIS"},    /* DCDC switch off                                   , */\
   { 0x980, "PSDR"},    /* Iddq test amplifier                               , */\
   { 0x991, "DCCV"},    /* Coil Value                                        , */\
   { 0x9b1, "CCFD"},    /* Selection CoolFluxClock                           , */\
   { 0x9d0, "ISEL"},    /* selection input 1 or 2                            , */\
   { 0x9e0, "IPLL"},    /* selection input PLL for lock                      , */\
   { 0xa02, "DOLS"},    /* Output selection dataout left channel             , */\
   { 0xa32, "DORS"},    /* Output selection dataout right channel            , */\
   { 0xa62, "SPKL"},    /* Selection speaker induction                       , */\
   { 0xa91, "SPKR"},    /* Selection speaker impedance                       , */\
   { 0xab3, "DCFG"},    /* DCDC speaker current compensation gain            , */\
   { 0xb07, "MTPK"},    /* 5Ah, 90d To access hidden registers (=Default for engineering), */\
   { 0xf00, "VDDD"},    /* mask flag_por for interupt generation             , */\
   { 0xf10, "OTDD"},    /* mask flag_otpok for interupt generation           , */\
   { 0xf20, "OVDD"},    /* mask flag_ovpok for interupt generation           , */\
   { 0xf30, "UVDD"},    /* mask flag_uvpok for interupt generation           , */\
   { 0xf40, "OCDD"},    /* mask flag_ocp_alarm for interupt generation       , */\
   { 0xf50, "CLKD"},    /* mask flag_clocks_stable for interupt generation   , */\
   { 0xf60, "DCCD"},    /* mask flag_pwrokbst for interupt generation        , */\
   { 0xf70, "SPKD"},    /* mask flag_cf_speakererror for interupt generation , */\
   { 0xf80, "WDD"},    /* mask flag_watchdog_reset for interupt generation  , */\
   { 0xfe0, "INT"},    /* enabling interrupt                                , */\
   { 0xff0, "INTP"},    /* Setting polarity interupt                         , */\
   { 0x62b0, "CIMTP"},    /* start copying all the data from i2cregs_mtp to mtp [Key 2 protected], */\
   { 0x7000, "RST"},    /* Reset CoolFlux DSP                                , */\
   { 0x7011, "DMEM"},    /* Target memory for access                          , */\
   { 0x7030, "AIF"},    /* Autoincrement-flag for memory-address             , */\
   { 0x7040, "CFINT"},    /* Interrupt CoolFlux DSP                            , */\
   { 0x7087, "REQ"},    /* request for access (8 channels)                   , */\
   { 0x710f, "MADD"},    /* memory-address to be accessed                     , */\
   { 0x720f, "MEMA"},    /* activate memory access (24- or 32-bits data is written/read to/from memory, */\
   { 0x7307, "ERR"},    /* cf error Flags                                    , */\
   { 0x7387, "ACK"},    /* acknowledge of requests (8 channels")"            , */\
   { 0x8000, "MTPOTC"},    /* Calibration schedule (key2 protected)             , */\
   { 0x8010, "MTPEX"},    /* (key2 protected)                                  , */\
    0xffff,"Unknown bitfield enum"    /* not found */\
};
#endif /* TFAFIELDNAMES_H_ */
