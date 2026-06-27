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
    _walkDir = dir;
    _state   = WALKING;
    _walkFrame = false;
    _targetX = (dir == RIGHT) ? (64 - MAPLE_CAT_W - 4) : 4;
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
    const int16_t clearW = MAPLE_CAT_W + 10;

    // Clear sky portion (cat top down to floor)
    int16_t skyH = MAPLE_FLOOR_Y - _lastY;
    if (skyH > 0)
        d->fillRect(clearX, _lastY, clearW, skyH, MAPLE_BG);

    // Restore floor strip
    d->fillRect(clearX, MAPLE_FLOOR_Y, clearW, 64 - MAPLE_FLOOR_Y, MAPLE_FLOOR);
}

void MapleCat::drawAt(int16_t x, int16_t y) {
    auto* d = Locator::getDisplay();

    // Ear tips (two single pixels)
    d->drawPixel(x + 1, y,     MAPLE_CAT);
    d->drawPixel(x + 6, y,     MAPLE_CAT);
    // Ear bases
    d->fillRect(x,     y + 1, 3, 1, MAPLE_CAT);
    d->fillRect(x + 5, y + 1, 3, 1, MAPLE_CAT);

    // Head block (rows 2–5)
    d->fillRect(x, y + 2, 8, 4, MAPLE_CAT);
    // Green eyes on head row 1 (y+3)
    d->drawPixel(x + 1, y + 3, MAPLE_EYE);
    d->drawPixel(x + 6, y + 3, MAPLE_EYE);

    // Body block (rows 6–9)
    d->fillRect(x, y + 6, 8, 4, MAPLE_CAT);

    // State-specific lower body
    switch (_state) {
        case SITTING:
            // Front paws tucked under body
            d->fillRect(x,     y + 10, 2, 2, MAPLE_CAT);
            d->fillRect(x + 6, y + 10, 2, 2, MAPLE_CAT);
            // Tail curled out to the left
            d->fillRect(x - 3, y + 11, 4, 1, MAPLE_CAT);
            break;

        case WALKING:
            // Alternating legs (2-frame animation)
            if (_walkFrame) {
                d->fillRect(x + 1, y + 10, 1, 3, MAPLE_CAT); // left leg forward
                d->fillRect(x + 6, y + 10, 1, 2, MAPLE_CAT); // right leg back
            } else {
                d->fillRect(x + 1, y + 10, 1, 2, MAPLE_CAT); // left leg back
                d->fillRect(x + 6, y + 10, 1, 3, MAPLE_CAT); // right leg forward
            }
            // Tail raised while walking
            d->drawPixel(x - 1, y + 9, MAPLE_CAT);
            d->drawPixel(x - 1, y + 8, MAPLE_CAT);
            d->drawPixel(x - 2, y + 7, MAPLE_CAT);
            break;

        case JUMPING:
            // Paws extended out to sides
            d->fillRect(x - 2, y + 9, 3, 1, MAPLE_CAT);
            d->fillRect(x + 8, y + 9, 3, 1, MAPLE_CAT);
            // Tail arching up
            d->drawPixel(x - 1, y + 8, MAPLE_CAT);
            d->drawPixel(x - 2, y + 6, MAPLE_CAT);
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
                _y        = _jumpBaseY;
                _state    = SITTING;
                _jumpDir  = UP;
            }
        }
    }

    drawAt(_x, _y);
}
