<div align="center">
  <img src="https://readme-typing-svg.herokuapp.com?font=Fira+Code&weight=600&size=35&pause=1000&color=FF0000&center=true&vCenter=true&width=800&lines=jester-gb+🃏;gameboy+in+your+terminal;unicode+braille+rendering;play+pokemon+via+ssh;written+in+raw+c%2B%2B" alt="Typing SVG" />
</div>

<div align="center">
  <img src="https://i.pinimg.com/originals/9d/1b/0e/9d1b0e92276C789b.gif" width="100%" height="2px" />
</div>

<br/>

<div align="center">
    (<b>dogshit</b> work in progress)<br/>
    <b>jester-gb</b> is a TUI emulator that renders nintendo glory in unicode braille.<br/>
    play tetris in your terminal while pretending to work.<br/>
    <b>linux + windows</b> 🐧🪟 | <b>c++17 bare metal magic</b> ⚡
</div>
<br/>
<div align="center">
  <img src="https://skillicons.dev/icons?i=cpp,linux,windows,cmake&theme=dark" />
</div>
<br/>

### 🔥 features that actually matter

- **braille rendering** - uses unicode braille patterns (`⣿`) so 160x144 pixels fit in your terminal
- **no bloat** - written in c++17 without heavy game engines. raw performance.
- **save states** - auto-saves battery RAM (.sav)
- **audio support** - 4-channel sound synthesis (linux/pulseaudio + windows/waveout)
- **palette system** - swap between classic green, vaporwave, or matrix styles

<br/>

<div align="center">
  <img src="https://i.pinimg.com/originals/9d/1b/0e/9d1b0e92276C789b.gif" width="100%" height="2px" />
</div>

### 🛠️ building this beast

**requirements:**
- a terminal that isn't garbage (utf-8 + truecolor support)
- cmake & a c++ compiler

**linux (arch):**

```bash
sudo pacman -S base-devel cmake libpulse
mkdir build && cd build
cmake .. && make -j12
```

**linux (debian/ubuntu):**

```bash
sudo apt install build-essential cmake libpulse-dev
mkdir build && cd build
cmake .. && make -j12
```

**windows (powershell):**

```bash
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
# audio works natively now. no extra deps needed.
```

### 🚀 usage

launch the menu or load a rom directly:

```bash
./jester-gb                  # opens tui file browser
./jester-gb roms/zelda.gb    # launches specific game
```

**flags:**

* `-p <0-4>` : set palette (0=white, 4=vaporwave)
* `-v <0-100>` : volume (don't blow your ears out)
* `-d` : debug mode (nerd stats)

### ⌨️ controls (vim vibes)

| key | action |
| --- | --- |
| `↑↓←→` | d-pad |
| `z` | **a button** |
| `x` | **b button** |
| `enter` | start |
| `space` | select |
| `esc` | pause / menu |
| `q` | quit (rage quit) |

### 🎨 palettes

change the vibe with `-p`:

| # | name | vibe |
| --- | --- | --- |
| `0` | **white** | high contrast, boring |
| `1` | **green** | classic dmg nostalgia |
| `2` | **amber** | fallout / crt terminal style |
| `3` | **blue** | cold visuals |
| `4` | **pink** | vaporwave / hotline miami |

### 🧠 architecture (for nerds)

```
src/
├── cpu/              # sharp lr35902 (the brain)
├── ppu/              # pixel processing (the pain)
├── apu/              # audio synthesis (multi-backend)
├── bus/              # memory mapping
├── tui/              # unicode magic renderer
└── main.cpp          # entry point
```

### ⚠️ disclaimer

i am not nintendo. i do not condone piracy. dump your own roms or use homebrew.
if this emulator crashes and burns your cpu, that's a skill issue.

### 🤝 contributing

PRs welcome. macos support needed (i don't own a mac).

<div align="center">
<img src="https://user-images.githubusercontent.com/73097560/115834477-dbab4500-a447-11eb-908a-139a6edaec5c.gif" width="100%"/>
</div>

<div align="center">
<b>made with 💀 by <a href="https://github.com/berochitiri">berochitiri</a></b>
</div>
