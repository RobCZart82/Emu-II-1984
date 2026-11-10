# Emu II 1984

[![Licenc: AGPL-3.0-only](https://img.shields.io/badge/License-AGPL--3.0--only-blue.svg)](LICENSE)
![Állapot: Kutatás / koncepció](https://img.shields.io/badge/Status-Research%20%2F%20Concept-orange)
[![English README](https://img.shields.io/badge/README-English-informational)](README.md)
[![Magyar README](https://img.shields.io/badge/README-Magyar-informational)](README_HU.md)

> **1984-es hardver. Nyolc hang. Nyers digitális karakter. Analóg hangzás.**

Az **Emu II 1984** független, nyílt forráskódú kutatási projekt. Célja az 1984-ben bemutatott **E-mu Emulator II** sampler **firmware-t is futtató, hardverközeli szoftveres emulációjának** vizsgálata, hosszú távon modern DAW-okban használható hangszerpluginként.

Az indusztriális synth-pop korai hangzásvilága, illetve George Orwell *1984* című regényének szikár atmoszférája inspirálja. A cél nem az eredeti hangszínek egyszerű, általános sample playerrel történő megszólaltatása, hanem a hangszer korlátainak, digitális mellékhatásainak és karakteres hangzásának reprodukálása.

> [!IMPORTANT]
> **Projektállapot: kutatási és architektúratervezési szakasz.** Ez a README fejlesztési koncepció, **nem működő Emulator II-emuláció bejelentése**. A leírás nem állítja, hogy a firmware-indítás, a hangmotor vagy a pluginbináris már elkészült volna.

## Vízió

A prioritás az **eredeti működés pontos rekonstrukciója, ahol ez műszakilag megoldható**:

- Az eredeti **main és scanner firmware** futtatása két emulált, névlegesen **4 MHz-es Z80 processzoron**.
- A **rendszerindítás, memórialapozás, periféria-I/O, lemezkezelés és vezérlés** rekonstruálása.
- **Nyolcszólamú mintalejátszás**, eredeti **8 bites kompandált hangút**, időzítés és DAC-karakterisztika.
- Az **SSM2045 analóg szűrő/VCA jelút**, burkológörbék és modulációk modellezése mérésekkel vagy kapcsolási rajzokkal alátámasztott DSP-vel.
- Régi lemezképek és hangbankfájlok kezelése anélkül, hogy minden mintát modern formátumba kellene alakítani.
- Hosszabb távon **VST3** (Windows/macOS), **AU** (macOS), esetleg **CLAP** plugin, eredeti hardver ihlette modern kezelőfelülettel.

**Tervezési alapelv:** az eredeti firmware emulált hardvert vezérel; a nem hozzáférhető vagy dokumentálatlan logikát külön, tesztelhető modellel kell pótolni. A Z80-ROM elindítása önmagában nem állít elő hangot.

## Tervezett architektúra

```text
DAW MIDI / automatizálás
          |
          v
Host adapter (VST3 / AU / opcionális CLAP) + GUI
          |
          v
Determinisztikus hardverütemező és eseménykezelés
          |
    +-----+-------------------+
    |                         |
    v                         v
Main Z80 (4 MHz)        Scanner Z80 (4 MHz)
ROM, RAM, I/O, DMA       Billentyűk, kezelőszervek
    |                         |
    +-----------+-------------+
                |
                v
Lemezvezérlő / OS / memórialapozás
                |
                v
8 hangú sampler + kompandálás + DAC modell
                |
                v
SSM2045 jellegű szűrő / VCA / analóg kimenet
                |
                v
DAW mintavételi frekvenciájú audio
```

A **valós idejű audio callbackben nem lehet blokkoló lemezműveletet vagy dinamikus memóriafoglalást végezni**. A hardverütemezést, újramintavételezést, állapotmentést és késleltetést külön ellenőrizni kell az első plugin-kiadás előtt.

## Firmware, lemezek és hiányzó hardverlogika

Az Emulator II esetén **külön kell kezelni a beépített firmware-t és a lemezről betöltött operációs rendszert**:

1. **Main CPU boot ROM** és **scanner CPU ROM** (a megfelelő verziópáros ellenőrzendő).
2. **OS/system floppy lemezkép**, amelyet az emulált meghajtórendszer tölt be.
3. **Hangbankok és minták** kompatibilis lemezképeken vagy bankfájlokban.
4. **Hanggenerátor-vezérlő logika**, beleértve a nyilvános referenciákból még hiányzó programozott logikát/mikrokódot.

A [MAME Emulator II skeleton drivere](https://github.com/mamedev/mame/blob/master/src/mame/emusys/emu2.cpp) két Z80-at és több perifériát dokumentál, de jelenleg **`MACHINE_NOT_WORKING | MACHINE_NO_SOUND`** állapotú. A két programozott logikai dump (`74s472.ic135`, `74s472.ic137`) **`NO_DUMP`** jelölésű. Ez fontos kutatási referencia, **nem kész hangszer-emulációs motor**.

### Vizsgálandó fájlformátumok

| Formátum | Szerep | Megjegyzés |
| --- | --- | --- |
| `.img` | Emulator II floppylemezkép | Tervezett kutatás / import |
| `.emuiifd` | Emulator II floppylemezkép | Tervezett kutatás / import |
| `.hfe` | Floppyemulátor-lemezkép | Tervezett kutatás / import |
| `.eii` | Kinyert Emulator II hangbankadat | **Bankfájl; nem azonos egy teljes rendszerindító floppyval** |

A formátumok megkülönböztetésének forrása a [ConvertWithMoss](https://github.com/git-moss/ConvertWithMoss) 20.3.0 kutatási eredménye. A tényleges kompatibilitás, az írás és a firmware-verziók támogatása **ebben a projektben még implementálásra és tesztelésre vár**.

## Fejlesztési ütemterv

Az alábbi mérföldkövek **tervezettek, nem befejezettek**.

| Mérföldkő | Terület | Elfogadási feltétel |
| --- | --- | --- |
| **M0** | Hardverdokumentáció és licencek | ROM/OS leltár, memóriatérkép, perifériák, forrás- és licenctábla |
| **M1** | Két Z80 alaparchitektúra | Determinisztikus CPU/perifériatesztek, main/scanner ROM indítási trace-ek |
| **M2** | Floppy, RAM és OS boot | Jogtisztán biztosított OS-lemezkép betöltése, reprodukálható OS/UI állapot |
| **M3** | Digitális hangmotor | 8 hang, kompandálás/DAC, loop és hangmagasságtesztek |
| **M4** | Analóg hangmodell | Szűrő/VCA/moduláció ellenőrzése rajzokkal, referenciafelvételekkel |
| **M5** | Pluginintegráció | VST3/AU prototípus, MIDI, presetállapot, offline render, blokkolásmentes audio |
| **M6** | Validálás és kiadások | Platformközi CI, hangregressziós tesztek, dokumentáció, reprodukálható build |

**Első mérnöki cél:** a valódi rendszerindítás ellenőrizhető, reprodukálható megvalósítása. A plugint csak a hangszer működésének megfelelő megértése után érdemes kiadásra késznek nevezni.

## Kutatási referenciák

- **[MAME — Emulator II driver](https://github.com/mamedev/mame/blob/master/src/mame/emusys/emu2.cpp):** hardvertérkép és perifériakutatás; a konkrét fájl fejlécében `BSD-3-Clause` szerepel. A MAME többi részének licence eltérhet.
- **[kosarev/z80](https://github.com/kosarev/z80):** MIT-licencű C++ Z80-mag, amelyet a projekt szintetikus tesztekkel értékel.
- **[ConvertWithMoss](https://github.com/git-moss/ConvertWithMoss):** LGPL-3.0-licencű samplerformátum-kutatás, Emulator II lemez/bank feldolgozással; **nincs integrálva**.
- **[JUCE](https://github.com/juce-framework/JUCE):** szóba jöhető pluginkeretrendszer, **AGPLv3 vagy kereskedelmi licenc** alatt; **nincs integrálva**.
- **[Steinberg VST3 SDK](https://github.com/steinbergmedia/vst3sdk):** az újabb SDK-verziók **MIT-licencűek**; **nincs integrálva**.

Külső kód átvétele előtt lásd: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Egy projekt hivatkozása nem jelenti a forráskód átvételét.

## Első lépések

A kutatási mag fordításához és a szintetikus tesztek futtatásához lásd a [fordítási útmutatót](docs/BUILD.md). A telepíthető pluginek az M5–M6 mérföldkőhöz tartoznak.

**Eredeti ROM-ot, OS-lemezképet, gyári hangkönyvtárat vagy más jogvédett hangmintát ne commitolj és ne terjessz megfelelő jogosultság nélkül.** A firmware és minták **nem tartoznak a projekt forráskódlicence alá**.

## Közreműködés

Szívesen fogadunk segítséget Z80-időzítés, kapcsolási rajzok, lemezformátumok, programozott logika, kompandálás/DAC és SSM2045-modellezés területén.

- Reprodukálható megfigyeléshez, kísérlethez vagy architektúrajavaslathoz nyiss **Issue-t**.
- Ellenőrizhető, megfelelő licencű kód vagy dokumentáció beküldéséhez használj **Pull Requestet**.
- Jelöld meg az átvett anyag forrását, szerzőjét, licencét és a tesztadatokat. Jogvédett ROM-ot vagy mintakészletet ne tölts fel.
- Törekedj tesztelhetőségre, és különítsd el a firmware-emulációt a valós idejű audio callbacktől.

## Licenc és jogi nyilatkozat

**A projekt saját forráskódja és dokumentációja:** **[GNU Affero General Public License v3.0 only (AGPL-3.0-only)](LICENSE)**, hacsak egy fájl ettől eltérően nem rendelkezik. Ez a licenc **megengedi a kereskedelmi felhasználást és terjesztést is a feltételei szerint**; a projekt nem vezet be „kizárólag nem kereskedelmi” korlátozást.

- **JUCE:** AGPLv3-as használata esetén a közös mű terjesztésekor teljesíteni kell az alkalmazandó AGPL-kötelezettségeket. A JUCE külön kereskedelmi licence **nem változtatja át automatikusan** a projekt AGPL-licencét.
- **Külső forráskódok:** szerzői jogaik és licencfeltételeik megmaradnak; import előtt külön ellenőrizendők.
- **Firmware, operációs rendszer, gyári presetek és hangminták:** nem tartoznak a projektlicenc alá, és nem tekinthetők szabadon terjeszthetőnek.
- **Védjegyek:** az E-mu és Emulator II megnevezések az eredeti hangszer azonosítását szolgálják. A projekt **független, nem hivatalos, és nincs kapcsolatban a védjegyjogosultakkal**.

A projekt kutatási és megőrzési célú, garanciavállalás nélkül. A szerzői és védjegyjogi megítélés országonként eltérhet; ez a README nem jogi tanácsadás.

---

*Emu II 1984 — megőrizni azokat a korlátokat, amelyek a legendás samplert egyedivé tették.*

## Fejlesztési alapok

[Build and tests](docs/BUILD.md) · [Current roadmap](docs/ROADMAP.md) · [Hardware map](docs/HARDWARE_MAP.md) · [Source audit](docs/SOURCE_AUDIT.md)

Az M0 része a scanner memóriamodellje, a fájlnév és méret szerinti formátumjelölés, valamint a [két Z80-példány szintetikus utasítástesztje](docs/CPU_DECISION.md). A firmware-indítás, a hangmotor és a pluginek későbbi mérföldkövek.
