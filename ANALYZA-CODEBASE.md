# Analýza codebase GLibpp

> **Stav k commitu:** `2fabb29` — *vyvoj - draw commandy: tagged union DrawCommand + DrawList, dispatch tabulka*, 2026-08-07, větev `main`
> **Datum analýzy:** 2026-08-07
> **Rozsah:** tříčlenný hloubkový audit celého `GLibpp/src/` (render backend + common; Core/Platform/App; Math/Geometry/Physics/Assets)
> **Předchozí analýza:** commit `0adc39a`, 2026-08-04 (viz git historie tohoto souboru)
> **Aktualizace 2026-08-08:** opravena concurrency trojice — krok 1 doporučeného pořadí (chyby 1–3 níže označeny ✅). Nový sdílený primitiv `Core::AtomicMailbox<T>` (`Engine/Core/Datastruct/AtomicMailbox.h`). Dále opravena chyba 9 (drawAxis NaN/UB) — osy se nyní klipují proti všem 6 rovinám frustumu.
> **Aktualizace 2026-08-10:** dokončen krok 2 — chyby 4, 5, 6 a 10 označeny ✅ (`Vec4::cross` čistý, `brake()` bez přestřelení a deadzone, `speedDown` škálované dt, bounds check indexů v `rasterizeMesh`). Dále sjednocena konvence interpolace (systémový problém 4) — hidden friends + ADL, zapsáno v CLAUDE.md. A **implementován depth buffer na DIB backendu** (krok 6 první polovina) — detail ve Výhledu, chyba 12 částečně opravena. Nakonec **rasterizér přepsán na hranové funkce nad subpixelovou mřížkou s top-left fill rule** (viz Výhled) — tím padl i bod „celočíselný `edgeInterp` bez subpixel přesnosti" z hodnocení render backendu.
> **Aktualizace 2026-08-11:** založen **testovací projekt `GLibppTests`** (systémový problém 7, 57 kontrol) a s ním opraveny chyby 7 a 8 — detail v příslušných bodech. Testy jsou součástí `glib-commit` gate.
> **Aktualizace 2026-08-11 (2):** **fragment shadery na DIB backendu** — shader = bezstavová struct (`setup` per trojúhelník + `shade` per pixel, C++20 concept), `rasterizeTriangleT<Shader>` inlinuje shade do vnitřní smyčky, výběr O(1) přes constexpr LUT (`fragmentFunction`, jeden nepřímý call per draw), `SetShader` command per průchod. Stínování (Lambert) se odstěhovalo z `rasterizeMesh` do `Backend/DIB/Shaders/ShaderLambert.h`. K tomu **`ShaderUniforms`** (per-draw konstanty, zatím invWidth/invHeight) tekoucí do setup i shade — `PixelInput` záměrně nese pixelové souřadnice (přesná identita, jako `gl_FragCoord`) a normalizované u/v si shader odvozuje sám (`ShaderUvDebug` to demonstruje). Testy 57 → 67. Navazující krok: per-vertex normály (Mesh atribut + interpolace) + SmoothLambert.
> **Aktualizace 2026-08-11 (3):** **texturovací pipeline** — `Mesh` má volitelný UV atribut, rasterizér interpoluje atributy perspektivně korektně (roviny u/w, v/w, 1/w jako hloubka; `PixelInput.uOverW/vOverW/invW`), `Assets::TextureData` + `Platform::ImageLoaderWin32` (GDI+, žádná externí závislost), residency = `ShaderResource` target bez GDI, binding přes command `SetTexture` → `ShaderUniforms.texture`, `ShaderTextured` (nearest + wrap). Testy 67 → 83 (vč. perspektivní korekce proti ručním barycentrikům a GDI+ dekódování s bottom-up flipem). **Poznatek o Debug budgetu:** vyplnění ~poloviny okna přes /Od rasterizér stojí >16,7 ms nezávisle na počtu trojúhelníků — plnoplošná texturovaná země proto neprošla a texturu nese malý panel; země zůstává drátěná (zapsáno v CLAUDE.md).
> **Aktualizace 2026-08-12:** **render-to-texture + end-of-frame capture** — residency textury je target, takže RT = `textureTargetGet` + `SetFramebuffer` pass (commit `5ca6969`); demo pak přešlo na levnější variantu: `Renderer::captureFrame` po submitu kopíruje hotový framebuffer (`targetCopyColor`, memcpy) a grayscale vizualizaci hloubky (`targetCopyDepthGray` → SIMD kernely `convertDepthToGray*` v `DeviceTargetDIB.h`, near světlá / far černá) do capture textur zobrazených na `fbPanel`/`depthPanel` s latencí 1 frame; capture targety sledují velikost okna (`syncCaptureSize`). Per-command instrumentace odhalila, že **plný line.obj stál 8–10,6 ms/frame** → instance přepnuta na wireframe. Testy 87 → 95 (konverzní kernely: přesné hodnoty, clamp, bit-shoda SIMD == skalár vč. ocásku).

---

# Celkové zhodnocení

**Architektura je kvalitní a udržitelná (8/10), implementace pod ní je nevyrovnaná (4–5/10). Celkem ~6/10.**

| Subsystém | Skóre | Poznámka |
|---|---|---|
| Core / Platform / App | **7/10** | SPSC most prokazatelně korektní; sráží ho Win32 error handling a hromadící se mrtvý kód |
| Render backend (DIB + common) | **5/10** | Švy (CRTP, handly, residency) 7–8; vnitřek rasterizéru prototyp s reálnými chybami (od 2026-08-10 s funkčním depth bufferem a bez NaN cesty v `drawAxis`) |
| Math / Geometry / Physics | **5/10** | Staré vrstvy nikdy nerevidované; Mtx4 je slabý základ |
| — z toho Assets samostatně | **8/10** | Nejlepší kus repa; jediná vada: tři mechanismy signalizace chyb |

Vzorec, který se opakuje napříč: **posledních 10 % se přeskočí.** Freshness flag triple bufferu sedí mimo RMW, které by ho udělalo neprůstřelným; `ResizeRequest` používá `relaxed` přesně tam, kde je potřeba `release`; Win32 povrch skoro nikde nekontroluje návratové hodnoty. Nic z toho není hluboká hniloba — každá položka níže je lokalizovaná, dobře pochopená oprava.

## Co se od minulé analýzy (0adc39a) vyřešilo

- Namespace konvence (adresář = namespace, kořen `GLibpp`) + migrace adresářů; zapsáno v CLAUDE.md.
- `CarTransformation` vytažena do `Car.h`, Scene.h je self-contained (bývalá největší fragilita).
- Mrtvá pole Scene, mrtvé větve `updateLogic`, duplicitní Camera, `RenderCommand/`+`Stencil/` — smazáno.
- `rollAngle` wrapován do [0, 2π) s wrap-aware Lerp přes `std::remainder` (audit to dnes uvádí jako silnou stránku).
- `Mtx4::inverse` fallback dosažitelný, `setIdentity` opraveno, `catch` by value opraven.
- Nové: **Assets modul** (IModelLoader/ModelImporter/ObjLoader/MeshAccess), follow kamera, high-res timer, snapshot interpolace s ringem N stavů, **draw command stream** (build→submit, dispatch tabulka).
- Hardcoded scéna v renderFrame (minule bod 4) je **z poloviny vyřešená**: submit fáze je generická, demo znalost zůstává v `buildDrawList` — zbytek čeká na MeshInstance/scene-object redesign.

## Co je silné (a čeho se držet)

- **Vláknový model** — ownership invariant `ZeroAllocTripleBuffer` je formálně ověřitelný (permutace indexů přes atomický exchange; reader a writer se nikdy nepotkají na stejném bufferu). Interpolace je samoopravná: alfa z reálných timestampů, `kInterpHistoryDepth` odvozená ze `kInterpDelayTicks` se `static_assert`, vlastní clamp diagnostika.
- **Assets** — loader seam (`IModelLoader` + `MeshAccess` jako jediný friend za všechny loadery), typed handly zdarma přes `StableRegistry<T>::Handle`, `freeze()` kontrakt vynucený, locale-odolný `from_chars`, ošetřené záporné indexy/`v//vn`/CRLF v .obj.
- **Render švy** — CRTP device bez vtable, generací validovaná residency (`meshRanges`), vlastnické pravidlo „offsety určuje výhradně backend" zapsané přímo u kódu, command stream s type-safe emit API.
- **Poctivé komentáře** — resize footgun je zdokumentovaný, ne schovaný; `static_assert(is_trivially_copyable_v<Scene>)` je strojově vynucený kontrakt publish cesty.

---

# Skutečné chyby (opravit)

## Concurrency (nejvyšší priorita — malé opravy, UB dopady)

1. ✅ **OPRAVENO 2026-08-08** — **`ResizeRequest` je datový race** (`Renderer.h:41-62`). `width`/`height` nejsou atomické; `active.store(false, relaxed)` po čtení → logické vlákno může začít zapisovat, zatímco render čte (UB, roztržený pár w/h), a `set()` doručený v okně mezi load a store se **tiše spolkne** (okno zůstane na staré velikosti). Při drag-resize se WM_SIZE sype nepřetržitě. *Provedený fix:* `ResizeRequest` je čistá datová struktura (w, h) přenášená přes nový generický `Core::AtomicMailbox<T>` — celý payload pakovaný `std::bit_cast` do jednoho `atomic<uint64_t>`, konzumace `exchange(0)`; pár cestuje nedělitelně, novější požadavek přepíše starší.

2. ✅ **OPRAVENO 2026-08-08** — **Triple buffer — ztracená notifikace** (`ZeroAllocTripleBuffer.h`). `has_new_data` je druhý atomic mimo RMW: interleaving `R.exchange < P.exchange < P.set(flag) < R.clear(flag)` nechá v `dirty_idx` čerstvý nepřečtený stav s flagem `false` → další publish ho vytáhne zpět a **jeden logický tick se tiše zahodí**. Vzácné (okno pár instrukcí), samoléčivé, ale je to díra v komponentě, jejíž jediná práce je korektnost — a clamp diagnostika v Rendereru ho neumí odlišit od jitteru scheduleru (instrumentace může částečně měřit tenhle bug). *Provedený fix:* freshness bit pakovaný přímo do `dirty_idx` (bit 2, indexy 0–2 v bitech 0–1; pojmenované helpery `withFresh`/`stripFresh`/`isFresh`), index i příznak cestují jedním `exchange`, druhý atomic zmizel.

3. ✅ **OPRAVENO 2026-08-08** — **`stop()` před `start()` = hang procesu** (`Renderer.h` runLoop + `App::run`). `running.start()` běží až po freeze + upload walku (v Debugu desítky ms); ESC/WM_CLOSE doručené dřív → `start()` přepíše `stop()`, smyčka běží věčně, `join()` se nevrátí. *Provedený fix:* `RunState` je tříhodnotový latch (`NotStarted/Running/Stopped`), `start()` = CAS jen z `NotStarted`, `stop()` je terminální — pozdější `start()` už stav neoživí.

## Math / fyzika

4. ✅ **OPRAVENO 2026-08-10** — **`Vec4::cross` je mutátor tvářící se jako čistá funkce** (`Vec4.h:28`, `Vec4.cpp:48-52`) — `a.cross(b)` přepíše `a`. Důsledek: `Mtx4::Slerp` (`Mtx4.h:113-114`) čte zničené `fwd` a **vrací nesmysl** (mrtvý, ale nabitý — 52 řádků inline v široce includovaném headeru); `Camera.h:92-93` přežívá jen náhodou. *Provedený fix:* `Vec4 cross(const Vec4&) const` — čistá funkce, žádná mutace příjemce; call sites beze změny (hodnotově identické), `Mtx4::Slerp` je tím matematicky správně, `Camera.h` už nespoléhá na náhodu.

5. ✅ **OPRAVENO 2026-08-10** — **`brake()` je pro velký faktor no-op** (`BicycleModel.h:116-117`): dva sekvenční `if`y (ne `else if`), první zmutuje hodnotu, kterou druhý testuje — `speed=0.15, faktor=1.0` skončí zpět na `0.15`. Plné brzdění nedělá nic. *Provedený fix:* dekrement směrem k nule bez přestřelení (`fabs(speed) <= decrement → 0`, jinak `copysign`), vzor shodný se `steerReset`. Zrušena i deadzone 0.1, kvůli které auto věčně dojíždělo ~0.1 m/s; parametr přejmenován `faktor` → `decrement` (je to absolutní hodnota).

6. ✅ **OPRAVENO 2026-08-10** — **`speedDown(0.01f)` není škálované dt** (`App.h:191`) — absolutní dekrement per tick mezi čtyřmi dt-škálovanými sousedy; změna `logicHz` změní jízdní chování. *Provedený fix:* `speedDown(0.6f * dt)` — pasivní dojezd ~0.6 m/s² nezávislý na `logicHz`; při 60 Hz je chování shodné s původním.

7. ✅ **OPRAVENO 2026-08-11** — **Unsigned underflow v grid builderech** (`MeshFactory.cpp:294`, `:353`): `size == 0` → `size - 1 == 4294967295` → OOM smyčka. `UpdateGridWave` má overflow fix (`size_t(size) * size`), sousední `reserve(size * size)` ne — oprava aplikovaná na 1 ze 3 míst. *Provedený fix:* `CreateGrid` i `CreateGridWave` mají guard `if (size < 2) return msh;` (pod 2×2 vrcholy není co triangulovat) a `reserve(size_t(size) * size)`. Testy v `GLibppTests` to drží. **Korekce původního nálezu:** modulo `% (segments * 2)` v `CreateCylinder` při `segments == 0` **nespadne** — všechny tři smyčky, které ho obsahují, mají podmínku `i < segments`, takže se nikdy nevyhodnotí; degenerovaný (ne padající) výsledek nicméně guard `segments < 3` teď odmítá, konzistentně s `CreateSphere`.

8. ✅ **OPRAVENO 2026-08-11** — **`Perspective` a `Orthographic` nesouhlasí v clip-space Z** (`Mtx4.cpp:503` vs `:513`): GL konvence [-1,1] vs [0,1] bez flipu. Projeví se jako „záhadný" bug při prvním použití ortho nebo depth bufferu. `inverseAffine` dělí nulou bez kontroly (`Mtx4.cpp:309`), zatímco `inverse` kontroluje — opačné bezpečnostní postoje v jedné třídě. *Provedený fix:* `Orthographic` má z-řádek `[0, 0, -2/(f-n), -(f+n)/(f-n)]`, takže mapuje `[near, far]` na NDC z v [-1, +1] stejně jako `Perspective` (a stejně jako předpokládá `kDepthFar`). `inverseAffine` dostal guard na `det == 0` s týmž fallbackem (nulová matice) jako `inverse` — přesná nula, ne epsilon, protože legitimně malé škálování má malý determinant a invertovat se dá. Obojí pokryto testy.

## Rasterizér / backend

9. ✅ **OPRAVENO 2026-08-08** — **`drawAxisImpl`: NaN → `(int)NaN` = UB** (`DeviceDIB.h:193-196` → `:443-459`). Když oba konce úsečky padnou mimo, nastaví se `w=0`, `divideW()` vyrobí NaN a `(int)` je UB; garbage souřadnice pak krmí **neclipnutý** Bresenham (multi-sekundový stall). A to „Clipping (jen jednou!)" (`:436-440`) kliupuje proti rovině `x + 0.5w` — **near-plane clipping v repu neexistuje nikde**, komentář lže (nejhorší komentář v celém repu). *Provedený fix:* každý segment osy se klipuje proti všem 6 rovinám frustumu v clip space (GL konvence, stejný poloprostor jako per-vertex test v `rasterizeMesh`) — near rovina garantuje `w ≥ nearZ > 0` (dělení nulou nemůže nastat), x/y roviny omezí NDC na [-1,1] (Bresenham dostává souřadnice uvnitř viewportu, stall nemožný). `clipSegmentWithPlane` přepsán na korektní kontrakt (vrací `bool` „celá venku" → segment se přeskočí; žádný degenerovaný `{0,0,0,0}`), `intersection` smazán. Platí ovšem stále: **trojúhelníky** near-plane clipping pořád nemají (viz Výhled).

10. ✅ **OPRAVENO 2026-08-10** — **Nevalidované čtení index bufferu** (`DeviceDIB.h:320-335`, `:371-381`): indexy z .obj jdou přímo do scratch bufferů, které jen rostou → vadný index čte **stale data předchozího meshe** (tichá vizuální korupce, nejhůř diagnostikovatelný failure mód). *Provedený fix:* trojúhelník s indexem mimo `vertexCount` se přeskočí (`continue`) hned po načtení indexů, s komentářem proč.

11. **`noexcept` lži → `std::terminate`** (`DeviceDIB.h:135-150`): `targetCreateImpl`/`targetResizeImpl` jsou `noexcept`, uvnitř `make_unique` + ctor, který hází. `StableRegistry::reset` navíc dělá `items[i].reset()` **před** `make_unique` → při throwu roztržené registry a pak terminate.

12. **Neinicializovaná čtení**: ~~`RenderTargetDescriptor` bez default member initializerů~~ (✅ NSDMI doplněny 2026-08-10); `LPVOID buf` v error handleru `DeviceTargetDIB.h:66-74` — při selhání `FormatMessageA` konstruuje `std::string` z neinicializovaného pointeru (crash uvnitř hlášení chyby) — **stále otevřené**. ~~`Depthbuffer24bit` deklaruje `TextureUsage::ColorAttachment`, `FramebufferRGBA32bit` žádá `RGBA32F` (128 bpp)~~ — ✅ obě opraveny 2026-08-10 a format je teď skutečně čtený (rozhoduje o residency targetu).

13. **`DeviceTargetDIB` porušuje rule of five**: vlastní HBITMAP/HDC/raw pointer, má destruktor, nemá `= delete` copy/move → náhodná kopie = double-free GDI handlů. (Vedlejší `Win32DC.h` to má učebnicově správně — a je celý nepoužitý.)

## Win32 povrch

14. **Návratové hodnoty se skoro nikde nekontrolují.** Nejzávažnější: `CreateWindowEx` (selhání po WM_NCCREATE → non-null handle na zničené okno, projde null checkem), `GetDC` (null HDC → `BitBlt(nullptr,…)` → trvale černé okno bez hlášky), `RegisterRawInputDevices` + `RIDEV_NOLEGACY` (selhání = **žádná klávesnice včetně ESC** = nezabitelné okno). `KEYMAP[vk]` indexované `USHORT` bez bounds checku (`WindowWin32.cpp:85`; zařízení umí poslat 0xFFFF).

15. **Release build je nechráněný**: `StableRegistry::get`/freeze kontrakt jsou jen asserty → v Release je stale handle null-deref bez diagnostiky. `execDrawMesh` validuje instanci, ale ne její `inst.mesh`.

---

# Systémové problémy (předělat)

## 1. Mrtvý kód se hromadí rychleji, než se maže

- **Mtx4.cpp: ~230 z 642 řádků bez volajícího** (inverse/transpose rodina, Orthographic, aritmetické operátory, file-static třetí kopie multiply smyčky) — a právě v mrtvé třetině žijí chyby 4 a 8.
- ~~**Depth-buffer plumbing horší než absence**: 4 vrstvy (descriptor → Renderer → DrawCommand `SetDepthbuffer` → ctx) s nulovým konzumentem, alokující plnou 32bpp DIB sekci + HDC + HBITMAP při každém resize.~~ ✅ **VYŘEŠENO 2026-08-10** — plumbing má konzumenta (viz Výhled níže), depth target je pole floatů bez GDI.
- `Win32DC.h` (kompletní správná RAII třída, nepoužitá), mrtvé AVX2 clear path (`clearMode` nemá setter; `__cpuid(7)` je navíc špatně — leaf 7 chce `__cpuidex`), `Vec4::Slerp`, `Mathematics::reciprocal*` (pesimizace s `cout` v headeru), mrtvé `MeshFactory::CreateGrid/Quad/Triangle/Sphere`, 8 mrtvých public metod WindowWin32/Keyboard, `timer10Hz` živící prázdnou lambdu. (`drawQuad` smazán 2026-08-10 spolu se změnou signatury `drawTriangle`.)
- Header hygiena: většina headerů není self-contained (kompilují se jen díky pořadí includů v Renderer.h); `StableRegistry.h` tahá `<iostream>` do celého enginu kvůli debug `operator<<`.

## 2. Tři mechanismy signalizace chyb v jednom modulu

Assets: výjimky (loader), asserty (registry, v Release zmizí), INVALID sentinel (deklarovaný, ale nikdy nevracený jako chyba). Volající nemá jednotný způsob, jak se zeptat „povedlo se?". **Doporučený kontrakt** (zapsat do CLAUDE.md): výjimky jen v init/load fázi, asserty na programátorské chyby + levná runtime validace handle v Release (return/skip), tiché skipy jen v render hot-path.

## 3. Duplikace přes hranici vláken a modulů

- Parametry vlny `(60, 0.2f, 0.05f)` v `App.h:244` i `Renderer.h:141` — komentář přiznává, že musí sedět ručně. Fix se sveze s extrakcí `setupDemoResources` (níže).
- 4×4 multiply smyčka 3× (`Mtx4.cpp`), grid triangulace 2× verbatim, wave vzorec 2×, dva fullscreen paths, `toWideString` reimplementovaný inline.
- **π v pěti pravopisech** (`3.1415926535f`, `3.14159265f`, `3.14159f` 2×, dvě definice `kTwoPi`), zatímco `Math::pi` sedí nepoužité — chybí `float` alias a `two_pi`.

## 4. ✅ VYŘEŠENO 2026-08-10 — Konvence interpolace je nahodilá

Šest typů, čtyři konvence (static member vs friend free function; `Camera::Slerp` je ve skutečnosti Lerp — jméno lže, a je **živé** v `Scene::Slerp`).

*Provedeno:* sjednoceno na **skryté friend funkce nalezené přes ADL** — `Vec4`, `Quaternion`, `Mtx4` a `CarWheel` převedeny ze statických metod, všechna volání jsou teď nekvalifikovaná (`Lerp(a, b, t)` / `Slerp(a, b, t)`). Falešný `Camera::Slerp` (tělo = `return Lerp(a,b,t)`) smazán; `Scene::Slerp` volá `Lerp` pro kameru a `Slerp` pro auto, takže jméno na každém řádku říká, co se skutečně děje. Konvence i její důvod zapsány do CLAUDE.md (sekce *Interpolation: hidden friends found by ADL*) a kanonický komentář do `Vec4.h`.

Zbývá jen jako **mrtvý kód** (krok 3, ne konvence): `Vec4::Slerp` bez volajícího; `Mtx4::Slerp` úmyslně ponechán — ožije, až world matice poputují do `Scene` (MeshInstance redesign).

## 5. BicycleModel: správná kinematika, hardcoded tuning

Quaternion heading + rotace kolem ICR je správný přístup. Ale `BicycleParams` má 4 pole a model hardcoduje ≥5 dalších tunables (steer return rate `4.0f`, tři různé epsilon „jedu rovně/stojím", žádný `maxSpeed` — `accelerate` je neomezený integrátor). Dvě nesynchronizované definice „jedu rovně" (`:84` epsilon vs `Car.h:138` isfinite). `getIcr()` + `tan()` se počítá 3–4× za frame.

## 6. App.h má 9 os změny

Okno/fullscreen, input+ESC policy, gameplay mapping, kamera, asset authoring, priority vláken, thread lifecycle, publish protokol, debug scaffolding — 349 řádků. **Pořadí extrakce podle výnosu:** ① `setupDemoResources` → `DemoScene.h` (zabije i duplikaci vlny), ② `updateFollowCamera` → controller vedle Camery, ③ `updateLogic` mapping → `CarController` (přirozené místo pro fix chyby 6), ④ window/OS bootstrap. Po ①–③ je App ~140 řádků čisté orchestrace.

## 7. ✅ ZALOŽENO 2026-08-11 — Chybí testy

Chyby 4, 5, 7, 8 (cross, brake, underflow, projekce) jsou přesně to, co jednořádkové unit testy chytí. Math + datastruct jsou deterministické, bez Win32 závislostí.

*Provedeno:* druhý console projekt **`GLibppTests`** (x64 only) s mini-runnerem bez frameworku (`TestRunner.h`: `check()`/`section()`, exit kód = počet selhání), zapojený do `glib-commit` rituálu jako samostatný gate. **57 kontrol, vše zelené** v Debug i Release. Testuje **skutečné hlavičky enginu**, ne kopie algoritmů — `test_rasterizer.cpp` staví reálné `DeviceTargetDIB` targety a volá shipovaný `RasterizerDIB`.

Pokryto: exkluzivní bezmezerové pokrytí + subpixel přesnost + depth test rasterizéru (tím je zachráněn dřívější scratchpad test, na který ukazovala CLAUDE.md a který by se ztratil), konvence clip-space Z obou projekcí, singulární inverze, čistota `Vec4::cross`, `Lerp`/`Slerp` přes ADL, konstantní úhlová rychlost `Quaternion::Slerp`, ortonormalita `Mtx4::Slerp`, degenerované vstupy a integrita index bufferu u `MeshFactory` (každý index míří do vertex bufferu — přesně předpoklad, na kterém stojí bounds check v rasterizéru).

**Poznámka k metodě:** u chyby 7 nešlo psát test před opravou — selhání nebyl assert, ale 4 miliardy iterací s OOM, takže test by se zasekl místo aby spadl. Guard a test tam vznikly zároveň. U chyby 8 by test selhal korektně (`Orthographic` vracel −0,01 místo −1), ale opravováno bylo rovněž v jednom kroku.

**Nepokryto zatím:** datové struktury (`ZeroAllocTripleBuffer`, `StableRegistry`, `ZeroAllocStateHistory`, `AtomicMailbox`) — SPSC chování se unit testem ověřuje obtížně, ale invarianty typu „handle po `reset()` zůstává platný" nebo „ring vrací stavy podle stáří" jsou přímočaré a stojí za doplnění. Dále `ObjLoader` (parsování ze stringu je čistá funkce, ideální kandidát) a `Car`/`CarWheel` wrap-aware interpolace přes hranici 0/2π.

## 8. Výhled: co budou stát plánované kroky

- ✅ **Depth buffer — HOTOVO 2026-08-10.** Depth target je u DIB backendu souvislé pole `float` bez jakéhokoliv GDI (`DeviceTargetDIB` se rozhoduje podle `isDepthFormat(descriptor.format)`), descriptor má opravené enum hodnoty (`Depth32F` + `DepthAttachment`, barva `RGBA8`) a NSDMI. `drawTriangle` bere z na vrchol a interpoluje ho rovinou v obrazovkovém prostoru — po perspektivním dělení je NDC z afinní funkcí x,y, takže je to **exaktní** a 1/w se pro hloubku nepotřebuje. Test „menší vyhrává", clear na `kDepthFar = 1.0f` jde přes tentýž SIMD fill jako barva (v Debugu podstatné: `std::fill_n` nad 0,5 M floatů stálo ~10 na 1% Low). Předpokládaný off-by-one problém se **nematerializoval**: inclusive spany znamenají, že sdílené hrany kreslí oba trojúhelníky, a se striktním `<` je druhý zápis zamítnut — žádné švy ani dvojité kreslení. Zato se objevily **artefakty na navazujících hranách** a měly dvě příčiny, obě opravené: rovina hloubky se fitovala na *zaokrouhlené* souřadnice vrcholů (dva trojúhelníky se společnou hranou tak dostaly měřitelně odlišné roviny a hádaly se o hloubku), a hloubka se vzorkovala v levém horním rohu pixelu místo v jeho **středu** (systematická chyba `0,5*(dzdx+dzdy)`, pro každý trojúhelník jiná). Rovina se teď počítá z přesných float souřadnic a vzorkuje se v `(x+0.5, y+0.5)`. Zrušen i přetížený sentinel `z == 0` pro neplatné vrcholy (kolidoval s legitimní NDC hloubkou 0) — nahradil ho vlastní příznakový buffer. **Drátěný model a osy hloubku ignorují** (debug overlay) — vědomé rozhodnutí, zdokumentované v CLAUDE.md.
- ✅ **Subpixel přesnost + exclusive fill rule — HOTOVO 2026-08-10.** Scanline chůze s celočíselným `edgeInterp` je pryč, `RasterizerDIB::drawTriangle` je přepsaný na **hranové funkce nad fixed-point mřížkou 1/256 pixelu** (stejná přesnost, jakou pro souřadnice používají GPU pipeline). Pokrytí rozhoduje test středu pixelu proti třem hranovým funkcím, doplněný **top-left fill rule** — hraniční pixel patří přesně jednomu ze dvou trojúhelníků, které hranu sdílejí. Fixed-point je tu podstatný, ne kosmetický: pro sdílenou hranu platí algebraicky `E_AB(p) == -E_BA(p)`, v celých číslech to platí na bit, ve floatu by zaokrouhlení mohlo pixel dát oběma (dvojitý zápis) nebo žádnému (švy). Vinutí se normalizuje podle znaménka plochy, průchod je incrementální (hranové funkce se jen přičítají) s předčasným ukončením řádku podle konvexity. Vrcholy se už nezaokrouhlují na celé pixely, takže zmizel i „vertex crawling".
  Ověřeno standalone testem (scratchpad `raster_test.cpp`, 24 kontrol): čtverec rozdělený diagonálou pokrývá **přesně** očekávanou množinu pixelů — žádný dvakrát, žádná mezera, nic mimo — a to i v případě, kdy leží přesně na středech pixelů a sdílená diagonála prochází jejich středy; vějíř 12 trojúhelníků se sdílenými hranami pod libovolnými úhly nemá jediný dvojitý pixel; subpixelový posun 0,3 px mění pokrytí; depth test drží bližší plochu nezávisle na pořadí kreslení.
- **Near-plane clipping** (těžší): dnešní per-vertex sentinel `z=0` + fixed-stride scratch buffery indexované původním vertex ID neumí vyjádřit nové vrcholy z clipu → přepis kroků 3–5 `rasterizeMesh`, ne vsuvka. `clipSegmentWithPlane` je od 2026-08-08 korektní (bool kontrakt, bez NaN), ale je segment-only — pro trojúhelníky (Sutherland–Hodgman, vznik nových vrcholů) je potřeba nový kód.
- **GL backend**: šev je správně, kontrakt nevynucený — chybí C++20 `concept RenderDevice` (nejvyšší páka: promění scavenger hunt v jeden čitelný blok), `DeviceTraits` je dnes fikce (definuje typy, které nic nepoužívá a DIB je kontradikuje), `present(TargetHandle)` je DIB-shaped (GL chce SwapBuffers bez targetu) a `ctx.depthbufferHandle` je DIB-ismus (GL chce attachment na FBO).

---

# Doporučené pořadí prací

| # | Co | Proč v tomhle pořadí |
|---|----|----|
| 1 | ✅ **hotovo 2026-08-08** — Concurrency trojice: `ResizeRequest` → `AtomicMailbox<T>` (atomic<u64>), freshness bit do `dirty_idx`, `RunState` latch | Nejvyšší závažnost/nejmenší diff; UB a hang pryč |
| 2 | ✅ **hotovo 2026-08-10** — `Vec4::cross` const, `brake()` bez přestřelení, `speedDown` dt-scale, bounds check indexů v rasterizéru (drawAxis NaN path už 2026-08-08) | Malé izolované opravy skutečných chyb |
| 3 | Velký úklid mrtvého kódu (Mtx4 třetina, depth plumbing zmrazit/zúžit, Win32DC rozhodnout, mrtvé factory/metody) + `= delete` na DeviceTargetDIB | Zadarmo; odstraní zavádějící scaffolding před kroky 5–6 |
| 4 | ~~Testovací projekt~~ (✅ 2026-08-11, 57 kontrol; datastruct + ObjLoader zbývají), ~~Slerp/Lerp konvence~~ (✅ 2026-08-10); zbývá sjednotit error kontrakt (zapsat do CLAUDE.md) a π/two_pi konstanty | Pojistka pro všechno další |
| 5 | Extrakce z App.h (DemoScene → zabije wave duplikaci; FollowCamera; CarController) + MeshInstance/scene-object redesign + bake draw listu | Odemyká data-driven scény; build fáze přestane znát demo |
| 6 | ~~Depth buffer~~ (✅ hotovo 2026-08-10) + near-plane clipping (mění rasterizer API) | Před dalším růstem scén |
| 7 | C++20 `concept RenderDevice`, srovnat `DeviceTraits` s realitou, pak GL backend | Až na vynucený, otestovaný kontrakt |

**Shrnuto:** projekt netrpí špatnou architekturou — švy jsou správně a nový kód (Assets, interpolace, command stream) na nich staví dobře. Trpí **prototypovými listovými vrstvami, které nikdo nerevidoval** (rasterizér, vnitřek Mtx4, Win32 error handling) a **mrtvým kódem, který aktivně mate** (falešný „clipping", fiktivní DeviceTraits, depth plumbing bez konzumenta). Kroky 1–3 jsou dohromady víkend práce a zvednou celek z ~6 na ~7,5; testy (krok 4) jsou jediná ochrana, aby se skóre už nepropadalo.
