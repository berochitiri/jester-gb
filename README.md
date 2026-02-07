<div align="center">
  <a href="https://github.com/berochitiri/jester-gb">
    <img src="https://i.ibb.co/j9zTT3kw/jester-no-bg.png" width="150" alt="Jester Logo" />
  </a>

  <br/>

  <img src="https://readme-typing-svg.herokuapp.com?font=Fira+Code&weight=600&size=35&pause=1000&color=FF0000&center=true&vCenter=true&width=800&lines=jester-gb+🃏;gameboy+in+your+terminal;unicode+braille+rendering;play+pokemon+via+ssh;written+in+raw+c%2B%2B" alt="Typing SVG" />
</div>

<div align="center">
    (<b>jester-emu</b> work in progress)<br/>
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
- **battery saves** - native .sav support (your pokemon are safe)
- **audio support** - 4-channel sound synthesis (linux/pulseaudio)
- **palette system** - swap between classic green, vaporwave, or matrix styles

### 📦 install (easy mode)

don't want to compile? just grab the package. updates automatically via your package manager.

**linux (debian / ubuntu):**

```bash
# add repo
echo 'deb https://files.distropack.dev/download/berochitiri/jester-gb/deb /' | sudo tee /etc/apt/sources.list.d/distropack_berochitiri_jester-gb.list

# add key & install
curl -fsSL 'https://files.distropack.dev/pubkey?user=berochitiri&project=jester-gb' | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/distropack_berochitiri_jester-gb.gpg > /dev/null
sudo apt update
sudo apt install jester-gb

```

**linux (arch):**

```bash
# add repo
echo -e "\n[distropack_berochitiri_jester-gb]\nServer = https://files.distropack.dev/download/berochitiri/jester-gb/pacman" | sudo tee -a /etc/pacman.conf

# add key and install
key=$(curl -fsSL 'https://files.distropack.dev/pubkey?user=berochitiri&project=jester-gb')
fingerprint=$(gpg --quiet --with-colons --import-options show-only --import --fingerprint <<< "${key}" | awk -F: '$1 == "fpr" { print $10 }')
sudo pacman-key --init
sudo pacman-key --add - <<< "${key}"
sudo pacman-key --lsign-key "${fingerprint}"
sudo pacman -Sy jester-gb

```

### 🛠️ building from source (hard mode)

if you prefer pain or want to hack on the code.

**requirements:**

- a terminal that isn't garbage (utf-8 + truecolor support) // (windows terminal on windows)
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

**windows:**

```powershell
# requires visual studio with c++ workload
mkdir build && cd build
cmake ..
cmake --build . --config Release

```

**macos:**

```bash
# install pulseaudio if you want audio
brew install pulseaudio
mkdir build && cd build
cmake ..
cmake --build . --config Release

```

### 🎮 windows notes

windows builds work great! audio + 60fps fully supported.
grab the prebuilt binary from **releases** or build from source above.

### 🚀 usage

launch the menu or load a rom directly:

```bash
./jester-gb                  # opens tui file browser
./jester-gb roms/zelda.gb    # launches specific game

```

**flags:**

- `-p <0-4>` : set palette (0=white, 4=vaporwave)
- `-v <0-100>` : volume (don't blow your ears out)
- `-d` : debug mode (nerd stats)

### ⌨️ controls

| key     | action           |
| ------- | ---------------- |
| `↑↓←→`  | d-pad            |
| `z`     | **a button**     |
| `x`     | **b button**     |
| `enter` | start            |
| `space` | select           |
| `esc`   | pause / menu     |
| `q`     | quit (rage quit) |

### 🎨 palettes

change the vibe with `-p`:

| #   | name      | vibe                         |
| --- | --------- | ---------------------------- |
| `0` | **white** | high contrast, boring        |
| `1` | **green** | classic dmg nostalgia        |
| `2` | **amber** | fallout / crt terminal style |
| `3` | **blue**  | cold visuals                 |
| `4` | **pink**  | vaporwave / hotline miami    |

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

PRs welcome. (big thanks to aceiii for testing on mac 😄)

<div align="center">
<img src="https://user-images.githubusercontent.com/73097560/115834477-dbab4500-a447-11eb-908a-139a6edaec5c.gif" width="100%"/>
</div>

<div align="center">
<b>made with 💀 by <a href="https://github.com/berochitiri">berochitiri</a></b>
</div>
