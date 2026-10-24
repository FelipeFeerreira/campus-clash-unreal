# Campus Clash — Unreal C++ Source Study

Selected C++ source and design data from an educational campus prototype built with **Unreal Engine 5.8**. The project explores interactive circuit exercises, calculus lessons and carrying/interaction systems.

> This repository is a source-code study. It does not include the original project's Content assets, maps, Blueprints or a standalone playable build. Cloning it alone does not produce a runnable Unreal project.

| | |
|:--|:--|
| **Live demo** | Unreal project — no hosted demo (source study) |
| **Screenshots** | Pending — see [docs/CAPTURE_GUIDE.md](docs/CAPTURE_GUIDE.md) |
| **Stack** | Unreal Engine · C++ |
| **License** | MIT for original code — [LICENSE](LICENSE) |

## Systems to explore

| Path | Focus |
| --- | --- |
| `Source/school/Circuit/` | Circuit exercise flow, widgets, progress and tests |
| `Source/school/CircuitLab/` | Workstations and laboratory gameplay |
| `Source/school/Calculus/` | Lesson logic, classroom interaction and tests |
| `Source/school/Interaction/` | Carryable objects and interaction components |
| `DesignData/` | CSV inputs and balancing data |

The source also contains Unreal template-derived character and game-mode code, including the `Variant_*` directories. These provide context for integration and are not presented as entirely original engine systems. Engine and third-party materials retain their applicable terms; this repository does not relicense them.

## Technical focus

- Separation of exercise data, gameplay actors and UI widgets.
- Validation of circuit inputs and explicit exercise-state transitions.
- Carry/pickup interactions and station-based challenges.
- C++ automation test sources alongside the gameplay modules.

## Integration requirements

To experiment with these modules, use an appropriately licensed Unreal installation and supply the project-specific assets and configuration in your own Unreal project. Review module dependencies in `Source/school/school.Build.cs` and all asset references before attempting integration. The included targets document the original module layout; no turnkey reconstruction is claimed.

## Verification

Source organization and publication contents were reviewed. This extracted repository was not compiled or play-tested. Automated tests in the source need the full Unreal project context. See [verification notes](docs/PORTFOLIO_VALIDATION.md).

---

## What I would do next

- Capture editor and gameplay screenshots that show which systems are original code.
- Document the Epic template boundaries file by file so the split is unambiguous.
- Turn the circuits/calculus hooks into a small standalone Unreal module.

## License

Original code and design data in this repository are released under the MIT license - see [LICENSE](LICENSE). Unreal Engine template-derived files (for example Source/school/Variant_*), engine code and third-party assets keep their original terms and are not relicensed here.
