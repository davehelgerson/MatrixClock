#ifndef CLOCKFACE_REGISTRY_H
#define CLOCKFACE_REGISTRY_H

#include <stdint.h>

/**
 * Single source of truth for clockface ordering, counts, and default menu labels.
 * Keep ClockApp::initClockfaces() aligned with FaceIndex below.
 */
namespace ClockfaceRegistry {

enum FaceIndex : int {
  FACE_MARIO     = 0,
  FACE_PACMAN    = 1,
  FACE_PACMAN2   = 2,
  FACE_POKEDEX   = 3,
  FACE_MAPLE     = 4,
  FACE_DAD       = 5,
  FACE_STARS     = 6,
  FACE_PIPES     = 7,
  FACE_ANALOG    = 8,
  FACE_MATRIX    = 9,
  FACE_INVADERS  = 10,
  FACE_JOKE      = 11,
  FACE_HOURGLASS = 12,
  FACE_SETTINGS  = 13,
  COUNT          = 14
};

static constexpr int DAD_INDEX        = FACE_DAD;
static constexpr int JOKE_INDEX       = FACE_JOKE;
static constexpr int SETTINGS_INDEX   = FACE_SETTINGS;
static constexpr int SELECTABLE_COUNT = FACE_SETTINGS; // All faces except always-on Settings

static_assert(COUNT <= 32, "Enabled faces mask supports up to 32 faces");

inline const char *defaultLabel(int index)
{
  static const char *const LABELS[COUNT] = {
    "Mario",
    "Pacman",
    "Pacman 2",
    "Pokedex",
    "Maple",
    "Dad",
    "Stars",
    "Pipes",
    "Analog",
    "Matrix",
    "Invaders",
    "Joke",
    "Hourglass",
    "Settings"
  };
  return (index >= 0 && index < COUNT) ? LABELS[index] : "";
}

}  // namespace ClockfaceRegistry

#endif
