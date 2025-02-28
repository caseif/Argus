pub struct ScreenSpace {
    left: f32,
    right: f32,
    top: f32,
    bottom: f32,
}

/// Controls how the screen space is scaled with respect to window
/// aspect ratio.
///
/// When configured as any value other than `None`, either the horizontal or
/// vertical axis will be "normalized" while the other is scaled. The normal
/// axis will maintain the exact bounds requested by the provided screen
/// space configuration, while the other will have its bounds changed such
/// that the aspect ratio of the screen space matches that of the window.
/// If the window is resized, the screen space will be updated in tandem.
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum ScreenSpaceScaleMode {
    /// Normalizes the screen space dimension with the minimum range.
    ///
    /// The bounds of the smaller window dimension will be exactly as
    /// configured. Meanwhile, the bounds of the larger dimension will be
    /// "extended" beyond what they would be in a square (1:1 aspect ratio)
    /// window such that regions become visible which would otherwise not be
    /// in the square window.
    ///
    /// For example, a typical computer monitor is wider than it is tall, so
    /// given this mode, the bounds of the screen space of a fullscreen
    /// window would be preserved in the vertical dimension, while the bounds
    /// in the horizontal dimension would be larger than usual (+/- 1.778 on
    /// a 16:9 monitor, exactly the ratio of the monitor dimensions).
    ///
    /// Meanwhile, a phone screen (held upright) is taller than it is wide,
    /// so it would see the bounds of the screen space extended in the
    /// vertical dimension instead.
    NormalizeMinDimension,

    /// Normalizes the screen space dimension with the maximum range.
    ///
    /// This is effectively the inverse of `NormalizedMinDimension`. The
    /// bounds of the screen space are preserved on the larger dimension, and
    /// "shrunk" on the smaller one. This effectively hides regions that
    /// would be visible in a square window.
    NormalizeMaxDimension,

    /// Normalizes the vertical screen space dimension.
    ///
    /// This invariably normalizes the vertical dimension of the screen
    /// space regardless of which dimension is larger. The horizontal
    /// dimension is grown or shrunk depending on the aspect ratio of the
    /// window.
    NormalizeVertical,

    /// Normalizes the horizontal screen space dimension.
    ///
    /// This invariably normalizes the horizontal dimension of the screen
    /// space regardless of which dimension is larger. The vertical
    /// dimension is grown or shrunk depending on the aspect ratio of the
    /// window.
    NormalizeHorizontal,

    /// Does not normalize screen space with respect to aspect ratio.
    ///
    /// Given an aspect ratio other than 1:1, the contents of the window will
    /// be stretched in one dimension or the other depending on which is
    /// larger.
    None,
}

impl Default for ScreenSpaceScaleMode {
    fn default() -> Self {
        ScreenSpaceScaleMode::NormalizeMinDimension
    }
}
