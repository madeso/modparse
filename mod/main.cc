#include "io.pb.h"
#include <fstream>
#include <string>
#include <cassert>
#include <iostream>
#include <sstream>

#include <google/protobuf/text_format.h>

#include "config.h"

bool LoadProto(google::protobuf::Message* t, const std::string& path) {
  using namespace std;
  assert(t);
#ifdef CONFIG_USE_BINARY
  std::fstream input(path.c_str(), std::ios::in
    | ios::binary
    );
  if (input) {
    if (t->ParseFromIstream(&input)) {
      return true;
    }
  }
  return false;
#else
  std::ifstream output(path.c_str());
  if (!output) return false;
  std::string data((std::istreambuf_iterator<char>(output)),
    std::istreambuf_iterator<char>());
  if (false == google::protobuf::TextFormat::ParseFromString(data, t)) return false;
  return true;
#endif
}

bool SaveProto(google::protobuf::Message* t, const std::string& path) {
  using namespace std;
  assert(t);
#ifdef CONFIG_USE_BINARY
  std::fstream output(path.c_str(), std::ios::out
    | ios::binary
    );
  if (output) {
    if (t->SerializeToOstream(&output)) {
      return true;
    }
  }
  return false;
#else
  std::ofstream output(path.c_str());
  std::string data;
  if (false == google::protobuf::TextFormat::PrintToString(*t, &data)) return false;
  // const auto data = t->SerializeAsString();
  if (data.empty()) return false;
  output << data;
  return true;
#endif
}

#define rassert(cond) do { assert(cond); if( (cond) == false ) {throw "failed to read";} } while(false)

#define assertsize(a,b) do { const int sizea = a; const int sizeb = b; assert(a == b); } while(false)

typedef unsigned short int word;
typedef unsigned int dword;

word readi2(FILE* f) {
  union D {
    char c[2];
    word i;
  };
  assertsize(sizeof(char)* 2, sizeof(word));
  assertsize(sizeof(char)* 4, sizeof(dword));

  D r;
  int read = fread(r.c, sizeof(char), 2, f);
  rassert(read == 2);
  std::swap(r.c[0], r.c[1]);
  return r.i;
}

// (Data repeated for each sample 1 - 15 or 1 - 31)
struct Sample{
  char name[22]; // Sample's name, padded with null bytes. If a name begins with a '#', it is assumed not to be an instrument name, and is probably a message.
  word length; // Sample length in words(1 word = 2 bytes).The first word of the sample is overwritten by the tracker, so a length of 1 still means an empty sample.See below for sample format.
  char finetune; // Lowest four bits represent a signed nibble(-8..7) which is the finetune value for the sample.Each finetune step changes the note 1 / 8th of a semitone.Implemented by switching to a different table of period - values for each finetune value.
  char volume; // Volume of sample.Legal values are 0..64.Volume is the linear difference between sound intensities. 64 is full volume, and the change in decibels can be calculated with 20 * log10(Vol / 64)
  word repeat_start; // Start of sample repeat offset in words.Once the sample has been played all of the way through, it will loop if the repeat length is greater than one.It repeats by jumping to this position in the sample and playing for the repeat length, then jumping back to this position, and playing for the repeat length, etc.
  word repeat_length; // Length of sample repeat in words.Only loop if greater than 1.

  std::vector<word> sample_data;

  void read(FILE* f) {
    int read;
    read = fread(name, sizeof(char), 22, f);
    rassert(read == 22);
    length = readi2(f);
    read = fread(&finetune, sizeof(char), 1, f);
    rassert(read == 1);
    read = fread(&volume, sizeof(char), 1, f);
    rassert(read == 1);
    repeat_start = readi2(f);
    repeat_length = readi2(f);
  }
};

int fpeek(FILE *stream)
{
  int c;
  c = fgetc(stream);
  ungetc(c, stream);
  return c;
}

enum Effect {
    FX_Arpeggio
  , FX_Slide_up
  , FX_Slide_down
  , FX_Slide_to_note
  , FX_Vibrato
  , FX_Continue_Slide_to_note_with_Volume_slide
  , FX_Continue_Vibrato_with_Volume_slide
  , FX_Tremolo
  , FX_Set_panning_position
  , FX_Set_sample_offset
  , FX_Volume_slide
  , FX_Position_Jump
  , FX_Set_volume
  , FX_Pattern_Break
  , FX_Set_filter_on_or_off
  , FX_Fineslide_up
  , FX_Fineslide_down
  , FX_Set_glissando_on_or_off
  , FX_Set_vibrato_waveform
  , FX_Set_finetune_value
  , FX_Loop_pattern
  , FX_Set_tremolo_waveform
  , FX_Unused
  , FX_Retrigger_sample
  , FX_Fine_volume_slide_up
  , FX_Fine_volume_slide_down
  , FX_Cut_sample
  , FX_Delay_sample
  , FX_Delay_pattern
  , FX_Invert_loop
  , FX_Set_speed
};

music::Effect C(Effect fx) {
  switch (fx)
  {
#define X(x) case x: return music::x;
    X(FX_Arpeggio)
    X(FX_Slide_up)
    X(FX_Slide_down)
    X(FX_Slide_to_note)
    X(FX_Vibrato)
    X(FX_Continue_Slide_to_note_with_Volume_slide)
    X(FX_Continue_Vibrato_with_Volume_slide)
    X(FX_Tremolo)
    X(FX_Set_panning_position)
    X(FX_Set_sample_offset)
    X(FX_Volume_slide)
    X(FX_Position_Jump)
    X(FX_Set_volume)
    X(FX_Pattern_Break)
    X(FX_Set_filter_on_or_off)
    X(FX_Fineslide_up)
    X(FX_Fineslide_down)
    X(FX_Set_glissando_on_or_off)
    X(FX_Set_vibrato_waveform)
    X(FX_Set_finetune_value)
    X(FX_Loop_pattern)
    X(FX_Set_tremolo_waveform)
    X(FX_Unused)
    X(FX_Retrigger_sample)
    X(FX_Fine_volume_slide_up)
    X(FX_Fine_volume_slide_down)
    X(FX_Cut_sample)
    X(FX_Delay_sample)
    X(FX_Delay_pattern)
    X(FX_Invert_loop)
    X(FX_Set_speed)
#undef X
  default:
    assert(false && "invalid case");
    return music::FX_Arpeggio;
  }
}

struct Num {
  word sample;
  word period;

  Effect effect;
  word x;
  word y;

  Num(dword da) {
    const word highnibble = 0xF << 12;

    const word lo = static_cast<word>(da);
    const word hi = static_cast<word>(da >> 16);
    
    sample = ((hi & highnibble) >> 8) | ((lo & highnibble) >> 12);
    period = hi & ~highnibble;
    word data = lo & ~highnibble;

    word effect_data[3];
    for (int i = 0; i < 3;  ++i) {
      effect_data[i] = (data >> 4*i) & 0xF;
    }

    const word i0 = effect_data[0];
    const word i1 = effect_data[1];
    const word i2 = effect_data[2];

         if( i0==0            ){ effect = FX_Arpeggio                                ; x=i1; y=i2; }
    else if( i0==1            ){ effect = FX_Slide_up                                ; x=i1; y=i2; }
    else if( i0==2            ){ effect = FX_Slide_down                              ; x=i1; y=i2; }
    else if( i0==3            ){ effect = FX_Slide_to_note                           ; x=i1; y=i2; }
    else if( i0==4            ){ effect = FX_Vibrato                                 ; x=i1; y=i2; }
    else if( i0==5            ){ effect = FX_Continue_Slide_to_note_with_Volume_slide; x=i1; y=i2; }
    else if( i0==6            ){ effect = FX_Continue_Vibrato_with_Volume_slide      ; x=i1; y=i2; }
    else if( i0==7            ){ effect = FX_Tremolo                                 ; x=i1; y=i2; }
    else if( i0==8            ){ effect = FX_Set_panning_position                    ; x=i1; y=i2; }
    else if( i0==9            ){ effect = FX_Set_sample_offset                       ; x=i1; y=i2; }
    else if( i0==10           ){ effect = FX_Volume_slide                            ; x=i1; y=i2; }
    else if( i0==11           ){ effect = FX_Position_Jump                           ; x=i1; y=i2; }
    else if( i0==12           ){ effect = FX_Set_volume                              ; x=i1; y=i2; }
    else if( i0==13           ){ effect = FX_Pattern_Break                           ; x=i1; y=i2; }
    else if( i0==14 && i1==0  ){ effect = FX_Set_filter_on_or_off                    ; x=i2; y=0 ; }
    else if( i0==14 && i1==1  ){ effect = FX_Fineslide_up                            ; x=i2; y=0 ; }
    else if( i0==14 && i1==2  ){ effect = FX_Fineslide_down                          ; x=i2; y=0 ; }
    else if( i0==14 && i1==3  ){ effect = FX_Set_glissando_on_or_off                 ; x=i2; y=0 ; }
    else if( i0==14 && i1==4  ){ effect = FX_Set_vibrato_waveform                    ; x=i2; y=0 ; }
    else if( i0==14 && i1==5  ){ effect = FX_Set_finetune_value                      ; x=i2; y=0 ; }
    else if( i0==14 && i1==6  ){ effect = FX_Loop_pattern                            ; x=i2; y=0 ; }
    else if( i0==14 && i1==7  ){ effect = FX_Set_tremolo_waveform                    ; x=i2; y=0 ; }
    else if( i0==14 && i1==8  ){ effect = FX_Unused                                  ; x=i2; y=0 ; }
    else if( i0==14 && i1==9  ){ effect = FX_Retrigger_sample                        ; x=i2; y=0 ; }
    else if( i0==14 && i1==10 ){ effect = FX_Fine_volume_slide_up                    ; x=i2; y=0 ; }
    else if( i0==14 && i1==11 ){ effect = FX_Fine_volume_slide_down                  ; x=i2; y=0 ; }
    else if( i0==14 && i1==12 ){ effect = FX_Cut_sample                              ; x=i2; y=0 ; }
    else if( i0==14 && i1==13 ){ effect = FX_Delay_sample                            ; x=i2; y=0 ; }
    else if( i0==14 && i1==14 ){ effect = FX_Delay_pattern                           ; x=i2; y=0 ; }
    else if( i0==14 && i1==15 ){ effect = FX_Invert_loop                             ; x=i2; y=0 ; }
    else if( i0==15           ){ effect = FX_Set_speed                               ; x=i1; y=i2; }
    else {
      assert(false && "invalid combo");
      throw "Invalid combo: par format or parser";
    }
  }
};

struct PatternData {
  std::vector<Num> num;
  void parse(dword* patterndata) {
    for (int chan = 0; chan < 4; ++chan) {
      for (int i = 0; i < 64; ++i) {
        num.push_back(Num(patterndata[64*chan + i]));
      }
    }
  }
};

struct ModFile {
  char name[20];

  std::vector<Sample> samples;

  char song_length;
  unsigned char pattern_table[128];

  std::vector<PatternData> pattern_data;

  void read(FILE* f) {
    int read;
    read = fread(name, sizeof(char), 20, f);
    rassert(read == 20);
    for (int i = 0; i < 31; ++i) {
      Sample sample;
      sample.read(f);
      samples.push_back(sample);
    }
    
    read = fread(&song_length, sizeof(char), 1, f);
    rassert(read == 1);
    char dummy; // historically set to 127
    read = fread(&dummy, sizeof(char), 1, f);
    rassert(read == 1);
    
    read = fread(pattern_table, sizeof(char), 128, f);
    rassert(read == 128);

    int next = fpeek(f);
    const bool more =
      next == 'M' ||
      next == 'F' ||
      next == '4' ||
      next == '6' ||
      next == '8';

    if (more) {
      char four[4];
      read = fread(four, sizeof(char), 4, f);
      rassert(read == 4);
    }

    int max = 0;
    for (int i = 0; i < 128; ++i) {
      if (pattern_table[i] > max) max = pattern_table[i];
    }

    for (int i = 0; i <= max; ++i) {
      unsigned char patterndata[1024];
      read = fread(patterndata, sizeof(char), 1024, f);
      rassert(read == 1024);

      PatternData data;
      data.parse(reinterpret_cast<dword*>(patterndata));
      pattern_data.push_back(data);
    }

    for (auto& s : samples) {
      for (int i = 0; i < s.length; ++i) {
        s.sample_data.push_back(readi2(f));
      }
    }
  }
};

void logic(const std::string& in, const std::string& out) {
  ModFile mod;
  FILE* f = fopen(in.c_str(), "rb");
  if (f) {
    mod.read(f);
  }
  fclose(f);

  music::Music music;
  music.set_name(mod.name);
  for (const auto& s : mod.samples) {
    music::Sample* sample = music.add_sample();
    sample->set_name(s.name);
    sample->set_length(s.length);
    sample->set_finetune(s.finetune);
    sample->set_volume(s.volume);
    sample->set_repeat_start(s.repeat_start);
    sample->set_repeat_length(s.repeat_length);
    for (const auto& d : s.sample_data) {
      sample->add_data(d);
    }
  }

  for (int i = 0; i < mod.song_length; ++i) {
    music.add_pattern(mod.pattern_table[i]);
  }

  for (const auto& d : mod.pattern_data) {
    auto* dd = music.add_pattern_data();
    for (const auto& n: d.num) {
      auto* nn = dd->add_num();
      nn->set_sample(n.sample);
      nn->set_period(n.period);
      nn->set_effect(C(n.effect));
      nn->set_x(n.x);
      nn->set_y(n.y);
    }
  }

  SaveProto(&music, out);
}

void main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (argc != 3) {
    std::cerr << "Usage:  " << argv[0] << " INPUT OUTPUT" << std::endl;
    return;
  }
  try {
    logic(argv[1], argv[2]);
  }
  catch (const std::exception& x) {
    std::cerr << x.what() << std::endl;
  }
  google::protobuf::ShutdownProtobufLibrary();
}
