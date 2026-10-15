# Design system

The reference direction uses charcoal and forest-green surfaces, a bright green
accent, restrained translucency, rounded corners and compact typography. Tokens
live in `Theme.qml`; UI code must not introduce ad-hoc brand colors when an
existing token applies.

- Primary accent: `#63DE87`
- Strong accent: `#2BA85B`
- Dark background: `#091210`
- Inter is the primary UI typeface.
- Control icons use simple rounded line geometry. Upstream application icons
  remain identifiable and come from Papirus or the application package.
- Animation duration is 120–200 ms. Reduced motion will disable positional
  transitions while preserving state changes.
- Unsupported controls such as display brightness are hidden instead of shown
  in a non-functional state.

The shell is the pixel-fidelity target. Third-party application content is
harmonised through Qt/GTK themes and is not treated as Project Fidan UI.

