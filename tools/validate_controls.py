#!/usr/bin/env python3
"""Check native relative mouse directions, Shift modes and the Escape pause menu on a QA display."""
import argparse
import json
import math
import os
import pathlib
import re
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', default='build-clean/cna_backrooms')
    parser.add_argument('--output', default='build/controls-qa')
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    output = (root / args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    # Fresh settings: a player's saved look sensitivity would change the angles.
    env.update(SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy', XDG_CONFIG_HOME=str(output / 'config'))
    states = []

    def xdo(*arguments):
        return subprocess.check_output(['xdotool', *map(str, arguments)],
                                       text=True, timeout=5).strip()

    def key(name):
        xdo('keydown', name)
        time.sleep(.12)
        xdo('keyup', name)

    with (output / 'game.log').open('w') as log:
        game = subprocess.Popen([str((root / args.game).resolve()), '--seed', '12345'],
                                cwd=root, env=env, stdout=log, stderr=log)
        try:
            time.sleep(3)
            window = xdo('search', '--pid', game.pid, '--name', 'cna-backrooms').splitlines()[0]

            def state(label):
                time.sleep(1.1)  # The native title updates once per second.
                title = xdo('getwindowname', window)
                position = re.search(r'pos ([\d.-]+),([\d.-]+)', title)
                angles = re.search(r'\| view ([\d.-]+),([\d.-]+)', title)
                if position is None or angles is None or 'loaded 25/25' not in title:
                    raise RuntimeError('missing ready controller telemetry: ' + title)
                result = {'label': label, 'position': list(map(float, position.groups())),
                          'angles': list(map(float, angles.groups())), 'title': title}
                states.append(result)
                print(label + ' | ' + title, flush=True)
                return result

            for label, dx, dy in [('right', 90, 0), ('left', -90, 0),
                                  ('up', 0, -90), ('down', 0, 90)]:
                before = state('before_' + label)
                xdo('mousemove_relative', '--', dx, dy)
                after = state(label)
                yaw = math.remainder(after['angles'][0] - before['angles'][0], 360)
                pitch = after['angles'][1] - before['angles'][1]
                if abs(yaw + math.degrees(dx * .0022)) > .3 or \
                        abs(pitch + math.degrees(dy * .0022)) > .3:
                    raise RuntimeError('nonconventional relative mouse direction: ' + label)

            def move(label, expected):
                key('r')
                before = state('before_' + label)
                xdo('keydown', 'w')
                time.sleep(2)
                xdo('keyup', 'w')
                after = state(label)
                dx = after['position'][0] - before['position'][0]
                dz = after['position'][1] - before['position'][1]
                if abs(dx - expected) > .4 or abs(dz) > .2:
                    raise RuntimeError('unexpected movement distance: ' + label)

            move('walk', 4.8)
            xdo('keydown', 'Shift_L')
            if '| run |' not in state('held_shift')['title']:
                raise RuntimeError('Shift did not enter running mode')
            if '| run |' not in state('still_held_shift')['title']:
                raise RuntimeError('holding Shift toggled repeatedly')
            xdo('keyup', 'Shift_L')
            move('run', 9.6)
            key('Shift_R')
            if '| walk |' not in state('second_shift')['title']:
                raise RuntimeError('second Shift did not return to walking')

            key('Escape')
            before = state('released_mouse')
            xdo('mousemove_relative', '--', 90, 0)
            if state('released_motion')['angles'] != before['angles']:
                raise RuntimeError('Escape did not open the menu and release mouse look')
            xdo('mousemove', '--window', window, 640, 360)
            subprocess.run(['import', '-window', window, str(output / 'menu.png')],
                           check=True, timeout=10)
            key('Escape')
            released = before
            before = state('recaptured_mouse')
            if before['angles'] != released['angles']:
                raise RuntimeError('resuming consumed absolute mouse coordinates as a delta')
            xdo('mousemove_relative', '--', 90, 0)
            after = state('recaptured_motion')
            if abs(math.remainder(after['angles'][0] - before['angles'][0], 360)
                   + math.degrees(90 * .0022)) > .3:
                raise RuntimeError('a second Escape did not resume mouse look')
            subprocess.run(['import', '-window', window, str(output / 'controls.png')],
                           check=True, timeout=10)
            # Quit from the pause menu: Resume, Settings, Back to the start, Quit.
            key('Escape')
            time.sleep(.2)
            for name in ('Down', 'Down', 'Down', 'Return'):
                key(name)
                time.sleep(.2)  # released for a few frames: separate presses
            if game.wait(timeout=5) != 0:
                raise RuntimeError('Quit from the menu failed')
            (output / 'summary.json').write_text(json.dumps(states, indent=2) + '\n')
            print('Conventional mouse look, walk/run toggle, pause menu and quit passed.', flush=True)
        finally:
            if game.poll() is None:
                xdo('keyup', 'w', 'Shift_L', 'Shift_R')
                game.terminate()
                game.wait(timeout=5)


if __name__ == '__main__':
    main()
