# third_party

External dependency source mirrors/vendor drops live here when used.

## Expected paths

- `third_party/SDL3` — optional SDL3 source checkout
- `third_party/bgfx.cmake` — optional bgfx.cmake checkout (builds bgfx + bx + bimg)

If these folders are absent, configure can fall back to system package (SDL only) or FetchContent when `RENDER_ALLOW_FETCHCONTENT=ON`.
