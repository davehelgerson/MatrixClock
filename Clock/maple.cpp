#include "maple.h"

MapleCat::MapleCat(int16_t x, int16_t y)
    : _x(x), _y(y), _lastX(x), _lastY(y),
      _state(SITTING), _walkDir(RIGHT), _jumpDir(UP),
      _jumpBaseY(y), _targetX(x), _walkFrame(false), _lastMillis(0)
{}

void MapleCat::init() {
    drawAt(_x, _y);
}

void MapleCat::walk(Direction dir) {
    if (_state != SITTING) return;
    _walkDir   = dir;
    _state     = WALKING;
    _walkFrame = false;
    _targetX   = (dir == RIGHT) ? (64 - MAPLE_CAT_W - 4) : 4;
}

void MapleCat::jump() {
    if (_state == JUMPING) return;
    _jumpBaseY = _y;
    _jumpDir   = UP;
    _state     = JUMPING;
}

void MapleCat::erase() {
    auto* d = Locator::getDisplay();
    int16_t clearX = _lastX - 5;
    if (clearX < 0) clearX = 0;
    const int16_t clearW = MAPLE_CAT_W + 12;

    int16_t skyH = MAPLE_FLOOR_Y - _lastY;
    if (skyH > 0)
        d->fillRect(clearX, _lastY, clearW, skyH, MAPLE_BG);

    d->fillRect(clearX, MAPLE_FLOOR_Y, clearW, 64 - MAPLE_FLOOR_Y, MAPLE_FLOOR);
}

void MapleCat::drawAt(int16_t x, int16_t y) {
    auto* d = Locator::getDisplay();

    // EARS: triangular -- single tip pixel widens to 3-pixel base
    d->drawPixel(x + 2, y,     MAPLE_CAT);
    d->drawPixel(x + 7, y,     MAPLE_CAT);
    d->fillRect(x + 1, y + 1, 3, 1, MAPLE_CAT);
    d->fillRect(x + 6, y + 1, 3, 1, MAPLE_CAT);

    // HEAD: 10 wide (wider than body for cat proportions)
    d->fillRect(x, y + 2, 10, 3, MAPLE_CAT);
    d->drawPixel(x + 2, y + 3, MAPLE_EYE);
    d->drawPixel(x + 7, y + 3, MAPLE_EYE);

    // CHIN: tapers 10 -> 8
    d->fillRect(x + 1, y + 5, 8, 1, MAPLE_CAT);

    // NECK: narrows to 6 wide, creates visible head/body distinction
    d->fillRect(x + 2, y + 6, 6, 1, MAPLE_CAT);

    // BODY: 8 wide, 4 rows
    d->fillRect(x + 1, y + 7, 8, 4, MAPLE_CAT);

    switch (_state) {
        case SITTING:
            // Two front paws tucked below body
            d->fillRect(x + 1, y + 11, 2, 2, MAPLE_CAT);
            d->fillRect(x + 7, y + 11, 2, 2, MAPLE_CAT);
            // Tail curls left: horizontal run then tip drops down
            d->fillRect(x - 3, y + 10, 4, 1, MAPLE_CAT);
            d->drawPixel(x - 3, y + 11, MAPLE_CAT);
            break;

        case WALKING:
            // Four legs: front pair (x+1, x+3) and rear pair (x+5, x+7)
            // Diagonal gait: front-left + rear-right move together
            if (_walkFrame) {
                d->fillRect(x + 1, y + 11, 1, 3, MAPLE_CAT);
                d->fillRect(x + 3, y + 11, 1, 2, MAPLE_CAT);
                d->fillRect(x + 5, y + 11, 1, 2, MAPLE_CAT);
                d->fillRect(x + 7, y + 11, 1, 3, MAPLE_CAT);
            } else {
                d->fillRect(x + 1, y + 11, 1, 2, MAPLE_CAT);
                d->fillRect(x + 3, y + 11, 1, 3, MAPLE_CAT);
                d->fillRect(x + 5, y + 11, 1, 3, MAPLE_CAT);
                d->fillRect(x + 7, y + 11, 1, 2, MAPLE_CAT);
            }
            // Tail raised in gentle arc
            d->drawPixel(x - 1, y + 10, MAPLE_CAT);
            d->drawPixel(x - 1, y + 9,  MAPLE_CAT);
            d->drawPixel(x - 2, y + 8,  MAPLE_CAT);
            d->drawPixel(x - 2, y + 7,  MAPLE_CAT);
            d->drawPixel(x - 3, y + 6,  MAPLE_CAT);
            break;

        case JUMPING:
            // All four legs extended straight down
            d->fillRect(x + 1, y + 11, 1, 3, MAPLE_CAT);
            d->fillRect(x + 3, y + 11, 1, 3, MAPLE_CAT);
            d->fillRect(x + 5, y + 11, 1, 3, MAPLE_CAT);
            d->fillRect(x + 7, y + 11, 1, 3, MAPLE_CAT);
            // Tail arches up and over
            d->drawPixel(x - 1, y + 9,  MAPLE_CAT);
            d->drawPixel(x - 2, y + 7,  MAPLE_CAT);
            d->drawPixel(x - 3, y + 5,  MAPLE_CAT);
            d->drawPixel(x - 3, y + 4,  MAPLE_CAT);
            break;
    }
}

void MapleCat::update() {
    if (_state == SITTING) return;

    uint16_t speed = (_state == JUMPING) ? MAPLE_JUMP_SPEED : MAPLE_WALK_SPEED;
    if ((unsigned long)(millis() - _lastMillis) < speed) return;
    _lastMillis = millis();

    _lastX = _x;
    _lastY = _y;
    erase();

    if (_state == WALKING) {
        _walkFrame = !_walkFrame;

        if (_walkDir == RIGHT) {
            _x++;
            if (_x >= _targetX) { _x = _targetX; _state = SITTING; }
        } else {
            _x--;
            if (_x <= _targetX) { _x = _targetX; _state = SITTING; }
        }
    }
    else if (_state == JUMPING) {
        if (_jumpDir == UP) {
            _y -= 2;
            if (_y <= _jumpBaseY - MAPLE_JUMP_HEIGHT) _jumpDir = DOWN;
        } else {
            _y += 2;
            if (_y >= _jumpBaseY) {
                _y       = _jumpBaseY;
                _state   = SITTING;
                _jumpDir = UP;
            }
        }
    }

    drawAt(_x, _y);
}
