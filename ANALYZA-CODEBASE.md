# Analýza codebase GLibpp

> **Stav k commitu:** `2fabb29` — *vyvoj - draw commandy: tagged union DrawCommand + DrawList, dispatch tabulka*, 2026-08-07, větev `main`
> **Datum analýzy:** 2026-08-07
> **Rozsah:** tříčlenný hloubkový audit celého `GLibpp/src/` (render backend + common; Core/Platform/App; Math/Geometry/Physics/Assets)
> **Předchozí analýza:** commit `0adc39a`, 2026-08-04 (viz git historie tohoto souboru)

---

# Celkové zhodnocení

**Architektura je kvalitní a udržitelná (8/10), implementace pod ní je nevyrovnaná (4–5/10). Celkem ~6/10.**

| Subsystém | Skóre | Poznámka |
|---|---|---|
| Core / Platform / App | **7/10** | SPSC most prokazatelně korektní; sráží ho Win32 error handling a hromadící se mrtvý kód |
| Render backend (DIB + common) | **5/10** | Švy (CRTP, handly, residency) 7–8; vnitřek rasterizéru prototyp s reálnými chybami |
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

1. **`ResizeRequest` je datový race** (`Renderer.h:41-62`). `width`/`height` nejsou atomické; `active.store(false, relaxed)` po čtení → logické vlákno může začít zapisovat, zatímco render čte (UB, roztržený pár w/h), a `set()` doručený v okně mezi load a store se **tiše spolkne** (okno zůstane na staré velikosti). Při drag-resize se WM_SIZE sype nepřetržitě. **Fix:** jeden `std::atomic<uint64_t>` s pakovaným w|h, `exchange(0)` při konzumaci.

2. **Triple buffer — ztracená notifikace** (`ZeroAllocTripleBuffer.h`). `has_new_data` je druhý atomic mimo RMW: interleaving `R.exchange < P.exchange < P.set(flag) < R.clear(flag)` nechá v `dirty_idx` čerstvý nepřečtený stav s flagem `false` → další publish ho vytáhne zpět a **jeden logický tick se tiše zahodí**. Vzácné (okno pár instrukcí), samoléčivé, ale je to díra v komponentě, jejíž jediná práce je korektnost — a clamp diagnostika v Rendereru ho neumí odlišit od jitteru scheduleru (instrumentace může částečně měřit tenhle bug). **Fix:** freshness bit pakovaný do `dirty_idx` (`idx | FRESH_BIT`), druhý atomic zmizí.

3. **`stop()` před `start()` = hang procesu** (`Renderer.h` runLoop + `App::run`). `running.start()` běží až po freeze + upload walku (v Debugu desítky ms); ESC/WM_CLOSE doručené dřív → `start()` přepíše `stop()`, smyčka běží věčně, `join()` se nevrátí. **Fix:** latch (stop je terminální), nebo `start()` v konstruktoru.

## Math / fyzika

4. **`Vec4::cross` je mutátor tvářící se jako čistá funkce** (`Vec4.h:28`, `Vec4.cpp:48-52`) — `a.cross(b)` přepíše `a`. Důsledek: `Mtx4::Slerp` (`Mtx4.h:113-114`) čte zničené `fwd` a **vrací nesmysl** (mrtvý, ale nabitý — 52 řádků inline v široce includovaném headeru); `Camera.h:92-93` přežívá jen náhodou. **Fix:** `Vec4 cross(const Vec4&) const` — opravit API, ne call sites.

5. **`brake()` je pro velký faktor no-op** (`BicycleModel.h:116-117`): dva sekvenční `if`y (ne `else if`), první zmutuje hodnotu, kterou druhý testuje — `speed=0.15, faktor=1.0` skončí zpět na `0.15`. Plné brzdění nedělá nic.

6. **`speedDown(0.01f)` není škálované dt** (`App.h:191`) — absolutní dekrement per tick mezi čtyřmi dt-škálovanými sousedy; změna `logicHz` změní jízdní chování. Parametr `brake(float faktor)` je navíc absolutní hodnota, ne faktor (a česky pojmenovaný identifikátor mimo konvenci).

7. **Unsigned underflow v grid builderech** (`MeshFactory.cpp:294`, `:353`): `size == 0` → `size - 1 == 4294967295` → OOM smyčka. `UpdateGridWave` má overflow fix (`size_t(size) * size`), sousední `reserve(size * size)` ne — oprava aplikovaná na 1 ze 3 míst. Příbuzné: `CreateCylinder` — modulo `% (segments * 2)` bez guardu = crash při `segments == 0`.

8. **`Perspective` a `Orthographic` nesouhlasí v clip-space Z** (`Mtx4.cpp:503` vs `:513`): GL konvence [-1,1] vs [0,1] bez flipu. Projeví se jako „záhadný" bug při prvním použití ortho nebo depth bufferu. `inverseAffine` dělí nulou bez kontroly (`Mtx4.cpp:309`), zatímco `inverse` kontroluje — opačné bezpečnostní postoje v jedné třídě.

## Rasterizér / backend

9. **`drawAxisImpl`: NaN → `(int)NaN` = UB** (`DeviceDIB.h:193-196` → `:443-459`). Když oba konce úsečky padnou mimo, nastaví se `w=0`, `divideW()` vyrobí NaN a `(int)` je UB; garbage souřadnice pak krmí **neclipnutý** Bresenham (multi-sekundový stall). A to „Clipping (jen jednou!)" (`:436-440`) kliupuje proti rovině `x + 0.5w` — **near-plane clipping v repu neexistuje nikde**, komentář lže (nejhorší komentář v celém repu).

10. **Nevalidované čtení index bufferu** (`DeviceDIB.h:320-335`, `:371-381`): indexy z .obj jdou přímo do scratch bufferů, které jen rostou → vadný index čte **stale data předchozího meshe** (tichá vizuální korupce, nejhůř diagnostikovatelný failure mód). Fix: `if (ia >= vertexCount) continue;`.

11. **`noexcept` lži → `std::terminate`** (`DeviceDIB.h:135-150`): `targetCreateImpl`/`targetResizeImpl` jsou `noexcept`, uvnitř `make_unique` + ctor, který hází. `StableRegistry::reset` navíc dělá `items[i].reset()` **před** `make_unique` → při throwu roztržené registry a pak terminate.

12. **Neinicializovaná čtení**: `RenderTargetDescriptor` bez default member initializerů (default-constructed = garbage width/format); `LPVOID buf` v error handleru `DeviceTargetDIB.h:66-74` — při selhání `FormatMessageA` konstruuje `std::string` z neinicializovaného pointeru (crash uvnitř hlášení chyby). `Depthbuffer24bit` deklaruje `TextureUsage::ColorAttachment` (copy-paste), `FramebufferRGBA32bit` žádá `RGBA32F` (128 bpp) — obě hodnoty dnes ignorované, obě špatně.

13. **`DeviceTargetDIB` porušuje rule of five**: vlastní HBITMAP/HDC/raw pointer, má destruktor, nemá `= delete` copy/move → náhodná kopie = double-free GDI handlů. (Vedlejší `Win32DC.h` to má učebnicově správně — a je celý nepoužitý.)

## Win32 povrch

14. **Návratové hodnoty se skoro nikde nekontrolují.** Nejzávažnější: `CreateWindowEx` (selhání po WM_NCCREATE → non-null handle na zničené okno, projde null checkem), `GetDC` (null HDC → `BitBlt(nullptr,…)` → trvale černé okno bez hlášky), `RegisterRawInputDevices` + `RIDEV_NOLEGACY` (selhání = **žádná klávesnice včetně ESC** = nezabitelné okno). `KEYMAP[vk]` indexované `USHORT` bez bounds checku (`WindowWin32.cpp:85`; zařízení umí poslat 0xFFFF).

15. **Release build je nechráněný**: `StableRegistry::get`/freeze kontrakt jsou jen asserty → v Release je stale handle null-deref bez diagnostiky. `execDrawMesh` validuje instanci, ale ne její `inst.mesh`.

---

# Systémové problémy (předělat)

## 1. Mrtvý kód se hromadí rychleji, než se maže

- **Mtx4.cpp: ~230 z 642 řádků bez volajícího** (inverse/transpose rodina, Orthographic, aritmetické operátory, file-static třetí kopie multiply smyčky) — a právě v mrtvé třetině žijí chyby 4 a 8.
- **Depth-buffer plumbing horší než absence**: 4 vrstvy (descriptor → Renderer → DrawCommand `SetDepthbuffer` → ctx) s nulovým konzumentem, alokující plnou 32bpp DIB sekci + HDC + HBITMAP při každém resize.
- `Win32DC.h` (kompletní správná RAII třída, nepoužitá), mrtvé AVX2 clear path (`clearMode` nemá setter; `__cpuid(7)` je navíc špatně — leaf 7 chce `__cpuidex`), `drawQuad`, `Vec4::Slerp`, `Mathematics::reciprocal*` (pesimizace s `cout` v headeru), mrtvé `MeshFactory::CreateGrid/Quad/Triangle/Sphere`, 8 mrtvých public metod WindowWin32/Keyboard, `timer10Hz` živící prázdnou lambdu.
- Header hygiena: většina headerů není self-contained (kompilují se jen díky pořadí includů v Renderer.h); `StableRegistry.h` tahá `<iostream>` do celého enginu kvůli debug `operator<<`.

## 2. Tři mechanismy signalizace chyb v jednom modulu

Assets: výjimky (loader), asserty (registry, v Release zmizí), INVALID sentinel (deklarovaný, ale nikdy nevracený jako chyba). Volající nemá jednotný způsob, jak se zeptat „povedlo se?". **Doporučený kontrakt** (zapsat do CLAUDE.md): výjimky jen v init/load fázi, asserty na programátorské chyby + levná runtime validace handle v Release (return/skip), tiché skipy jen v render hot-path.

## 3. Duplikace přes hranici vláken a modulů

- Parametry vlny `(60, 0.2f, 0.05f)` v `App.h:244` i `Renderer.h:141` — komentář přiznává, že musí sedět ručně. Fix se sveze s extrakcí `setupDemoResources` (níže).
- 4×4 multiply smyčka 3× (`Mtx4.cpp`), grid triangulace 2× verbatim, wave vzorec 2×, dva fullscreen paths, `toWideString` reimplementovaný inline.
- **π v pěti pravopisech** (`3.1415926535f`, `3.14159265f`, `3.14159f` 2×, dvě definice `kTwoPi`), zatímco `Math::pi` sedí nepoužité — chybí `float` alias a `two_pi`.

## 4. Konvence interpolace je nahodilá

Šest typů, čtyři konvence (static member vs friend free function; `Camera::Slerp` je ve skutečnosti Lerp — jméno lže, a je **živé** v `Scene::Slerp`). Sjednotit na friend free `Slerp/Lerp` (forma, kterou používá Scene→Car→Camera řetěz) a mrtvé `Mtx4::Slerp`/`Vec4::Slerp` smazat.

## 5. BicycleModel: správná kinematika, hardcoded tuning

Quaternion heading + rotace kolem ICR je správný přístup. Ale `BicycleParams` má 4 pole a model hardcoduje ≥5 dalších tunables (steer return rate `4.0f`, tři různé epsilon „jedu rovně/stojím", žádný `maxSpeed` — `accelerate` je neomezený integrátor). Dvě nesynchronizované definice „jedu rovně" (`:84` epsilon vs `Car.h:138` isfinite). `getIcr()` + `tan()` se počítá 3–4× za frame.

## 6. App.h má 9 os změny

Okno/fullscreen, input+ESC policy, gameplay mapping, kamera, asset authoring, priority vláken, thread lifecycle, publish protokol, debug scaffolding — 349 řádků. **Pořadí extrakce podle výnosu:** ① `setupDemoResources` → `DemoScene.h` (zabije i duplikaci vlny), ② `updateFollowCamera` → controller vedle Camery, ③ `updateLogic` mapping → `CarController` (přirozené místo pro fix chyby 6), ④ window/OS bootstrap. Po ①–③ je App ~140 řádků čisté orchestrace.

## 7. Chybí testy — pořád, a chyby to potvrzují

Chyby 4, 5, 7, 8 (cross, brake, underflow, projekce) jsou přesně to, co jednořádkové unit testy chytí. Math + datastruct jsou deterministické, bez Win32 závislostí. **Jak:** druhý console projekt `GLibppTests` s mini-runnerem (pár assert funkcí, žádný framework), zapojený do `glib-commit` rituálu.

## 8. Výhled: co budou stát plánované kroky

- **Depth buffer** (~den práce): ctx handle už na backend doteče, ale současné plumbing aktivně mate — depth target je HBITMAP-backed, descriptor má špatné enum hodnoty, `drawTriangle` bere jen x,y (chybí barycentrika/1w interpolace) a off-by-one v scanline smyčce (`y1` kreslený 2×, inclusive spany) se stane viditelným artefaktem hned s prvním Z-testem.
- **Near-plane clipping** (těžší): dnešní per-vertex sentinel `z=0` + fixed-stride scratch buffery indexované původním vertex ID neumí vyjádřit nové vrcholy z clipu → přepis kroků 3–5 `rasterizeMesh`, ne vsuvka. Stávající `clipSegmentWithPlane`/`intersection` nejsou reusable (segment-only, špatná rovina, NaN bug).
- **GL backend**: šev je správně, kontrakt nevynucený — chybí C++20 `concept RenderDevice` (nejvyšší páka: promění scavenger hunt v jeden čitelný blok), `DeviceTraits` je dnes fikce (definuje typy, které nic nepoužívá a DIB je kontradikuje), `present(TargetHandle)` je DIB-shaped (GL chce SwapBuffers bez targetu) a `ctx.depthbufferHandle` je DIB-ismus (GL chce attachment na FBO).

---

# Doporučené pořadí prací

| # | Co | Proč v tomhle pořadí |
|---|----|----|
| 1 | Concurrency trojice: `ResizeRequest` → atomic<u64>, freshness bit do `dirty_idx`, `RunState` latch | Nejvyšší závažnost/nejmenší diff; UB a hang pryč |
| 2 | `Vec4::cross` const, `brake()` else-if+clamp, `speedDown` dt-scale, bounds check indexů v rasterizéru, drawAxis NaN path | Malé izolované opravy skutečných chyb |
| 3 | Velký úklid mrtvého kódu (Mtx4 třetina, depth plumbing zmrazit/zúžit, Win32DC rozhodnout, mrtvé factory/metody) + `= delete` na DeviceTargetDIB | Zadarmo; odstraní zavádějící scaffolding před kroky 5–6 |
| 4 | Testovací projekt math + datastruct; sjednotit error kontrakt (zapsat do CLAUDE.md); π/two_pi konstanty; Slerp/Lerp konvence | Pojistka pro všechno další |
| 5 | Extrakce z App.h (DemoScene → zabije wave duplikaci; FollowCamera; CarController) + MeshInstance/scene-object redesign + bake draw listu | Odemyká data-driven scény; build fáze přestane znát demo |
| 6 | Depth buffer + near-plane clipping (mění rasterizer API) | Před dalším růstem scén |
| 7 | C++20 `concept RenderDevice`, srovnat `DeviceTraits` s realitou, pak GL backend | Až na vynucený, otestovaný kontrakt |

**Shrnuto:** projekt netrpí špatnou architekturou — švy jsou správně a nový kód (Assets, interpolace, command stream) na nich staví dobře. Trpí **prototypovými listovými vrstvami, které nikdo nerevidoval** (rasterizér, vnitřek Mtx4, Win32 error handling) a **mrtvým kódem, který aktivně mate** (falešný „clipping", fiktivní DeviceTraits, depth plumbing bez konzumenta). Kroky 1–3 jsou dohromady víkend práce a zvednou celek z ~6 na ~7,5; testy (krok 4) jsou jediná ochrana, aby se skóre už nepropadalo.
