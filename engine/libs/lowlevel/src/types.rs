#[derive(PartialEq)]
pub enum TriState {
    Unset,
    True,
    False
}

impl TriState {
    pub fn is_set(&self) -> bool {
        *self != TriState::Unset
    }
}

impl Into<bool> for TriState {
    fn into(self) -> bool {
        self == TriState::True
    }
}

impl Into<TriState> for bool {
    fn into(self) -> TriState {
        match self {
            true => TriState::True,
            false => TriState::False
        }
    }
}
