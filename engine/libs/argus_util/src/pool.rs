use std::collections::LinkedList;
use std::error::Error;
use std::fmt::{Debug, Display, Formatter};
use std::marker::PhantomData;
use std::ptr;
use std::mem::MaybeUninit;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct Handle {
    index: u32,
    version: u32,
}

/// A structure for storing many values of a specific type while minimizing
/// memory allocations/deallocations.
pub struct ValuePool<V: Sized> {
    slot_interval: usize,
    slots_per_chunk: usize,
    storage: ValuePoolStorage,
    phantom_v: PhantomData<V>,
}

struct ValuePoolStorage {
    chunks: Vec<Box<[u8]>>,
    empty_indices: LinkedList<u32>,
}

impl<V> Default for ValuePool<V> {
    fn default() -> Self {
        Self::new()
    }
}

impl<V> ValuePool<V> {
    /// Constructs a new ValuePool capable of storing values of type [V].
    pub fn new() -> Self {
        let slot_interval = next_aligned_value(
            size_of::<Slot<V>>(), align_of::<Slot<V>>()
        );

        // the underlying assumption here is that really massive types will
        // probably only have a few instances floating around at any given
        // point, so having fewer slots per chunk won't matter as much
        let slots_per_chunk = if slot_interval <= 64 {
            // one 4 KiB page (at least 64 slots)
            4096 / slot_interval
        } else if slot_interval <= 128 {
            // two 4 KiB pages (at least 64 slots)
            4096 / slot_interval
        } else if slot_interval <= 256 {
            // two 4 KiB pages (at least 32 slots)
            8192 / slot_interval
        } else if slot_interval <= 512 {
            // four 4 KiB pages (at least 32 slots)
            16384 / slot_interval
        } else {
            // massive type, alloc just enough for 16 slots per chunk
            slot_interval * 16
        };

        assert!(slots_per_chunk < i32::MAX as usize);

        let mut pool = Self {
            slot_interval,
            slots_per_chunk,
            storage: ValuePoolStorage {
                chunks: Vec::new(),
                empty_indices: LinkedList::new(),
            },
            phantom_v: Default::default(),
        };

        pool.alloc_chunk().expect("Allocation failed");

        pool
    }

    /// Inserts a new value into the pool and returns a [Handle] to the inserted
    /// value.
    pub fn insert(&mut self, value: V) -> Handle {
        let (_, handle) = self.insert_impl(value);
        handle
    }

    /// Inserts a new value into the pool and invokes the given callback with a
    /// [Handle] and a reference to the inserted value, then returns the handle.
    pub fn insert_and_then<F>(&mut self, value: V, f: F) -> Handle
        where F: FnOnce(&mut V, Handle) {
        let (inserted, handle) = self.insert_impl(value);
        f(inserted, handle);
        handle
    }

    fn insert_impl(&mut self, value: V) -> (&mut V, Handle) {
        let index = match self.storage.empty_indices.pop_front() {
            Some(index) => index,
            None => {
                self.alloc_chunk().expect("Allocation failed")
            }
        };

        let slot = self.get_slot_mut(index).unwrap();
        debug_assert!(!slot.is_occupied());
        let version = slot.increment_and_get_version();
        slot.value.write(value);
        slot.set_occupied(true);

        // SAFETY: We populate the slot immediately before this statement.
        let val_ref = unsafe { slot.value.assume_init_mut() };
        let handle = Handle { index, version };

        ( val_ref, handle )
    }

    pub fn remove(&mut self, handle: Handle) -> Option<V> {
        if let Some(slot) = self.get_slot_mut(handle.index)
            .filter(|slot| slot.is_occupied() && slot.get_version() == handle.version) {
            slot.set_occupied(false);

            // SAFETY: The slot value will not be used again until the occupied
            //         flag is set, at which point its contents will already
            //         have been overwritten.
            let old_value = unsafe {
                Some(slot.value.assume_init_read())
            };

            self.storage.empty_indices.push_back(handle.index);

            old_value
        } else {
            None
        }
    }

    #[must_use]
    pub fn get(&self, handle: Handle) -> Option<&V> {
        self.get_slot(handle.index)
            .filter(|slot| slot.is_occupied() && slot.get_version() == handle.version)
            .map(|slot| {
                // SAFETY: Slot::is_occupied will return true iff the slot value
                //         is populated (initialized).
                unsafe {
                    slot.value.assume_init_ref()
                }
            })
    }

    #[must_use]
    pub fn get_mut(&mut self, handle: Handle) -> Option<&mut V> {
        self.get_slot_mut(handle.index)
            .filter(|slot| slot.is_occupied() && slot.get_version() == handle.version)
            .map(|slot| {
                // SAFETY: Slot::is_occupied will return true iff the slot value
                //         is populated (initialized).
                unsafe {
                    slot.value.assume_init_mut()
                }
            })
    }

    fn alloc_chunk(&mut self) -> Result<u32, AllocError> {
        let new_chunk_index = self.storage.chunks.len();
        let first_slot_index = new_chunk_index * self.slots_per_chunk;
        let last_slot_index = first_slot_index + self.slots_per_chunk - 1;

        if last_slot_index > u32::MAX as usize {
            return Err(AllocError::new("Exceeded max slot index"));
        }

        // SAFETY: The chunk layout cannot have size 0 so long as slot_interval
        //         is non-zero. slot_interval derives its size from the size of
        //         the Slot struct which cannot be zero-sized because of its
        //         version field.
        let new_chunk = vec![0u8; self.slot_interval * self.slots_per_chunk].into_boxed_slice();
        self.storage.chunks.push(new_chunk);
        self.storage.empty_indices.extend((first_slot_index as u32 + 1)..=last_slot_index as u32);

        Ok(first_slot_index as u32)
    }

    /// Returns a reference to the slot at the given index, or None if the index
    /// is not valid for this pool.
    fn get_slot(&self, index: u32) -> Option<&Slot<V>> {
        let chunk_index = index as usize / self.slots_per_chunk;
        if chunk_index >= self.storage.chunks.len() {
            return None;
        }
        let chunk = &self.storage.chunks[chunk_index];

        let index_in_chunk = index as usize % self.slots_per_chunk;
        let offset_in_chunk = index_in_chunk * self.slot_interval;
        assert!(offset_in_chunk <= isize::MAX as usize);
        // SAFETY: The chunk index can't be more than slots_per_chunk - 1,
        //         and the block of memory is only ever addressed with offsets
        //         that are multiples of slot_interval.
        unsafe {
            Some(&*chunk.as_ptr().add(offset_in_chunk).cast())
        }
    }

    /// Returns a mutable reference to the slot at the given index, or None if
    /// the index is not valid for this pool.
    fn get_slot_mut(&mut self, index: u32) -> Option<&mut Slot<V>> {
        self.get_slot(index).map(|slot| {
            // SAFETY: The returned Slot is owned by `self` which is mutable in
            //         this context.
            unsafe {
                &mut *ptr::addr_of!(*slot).cast_mut()
            }
        })
    }
}

#[must_use]
struct Slot<T> {
    version: u32,
    value: MaybeUninit<T>,
}

// first bit is used as an occupied flag, i.e. a 1 indicates the slot contains
// a value and a 0 indicates it is empty
const VERSION_MASK: u32 = 0x7FFF_FFFF;
const OCCUPIED_FLAG_MASK: u32 = 0x8000_0000;

const MAX_VERSION: u32 = 0x7FFF_FFFF;

impl<T> Slot<T> {
    #[must_use]
    fn get_version(&self) -> u32 {
        self.version & VERSION_MASK
    }

    fn increment_and_get_version(&mut self) -> u32 {
        if (self.version & VERSION_MASK) == MAX_VERSION {
            panic!("Max slot version exceeded");
        }
        self.version += 1;
        self.version
    }

    #[must_use]
    fn is_occupied(&self) -> bool {
        self.version & OCCUPIED_FLAG_MASK != 0
    }

    fn set_occupied(&mut self, occupied: bool) {
        self.version = if occupied { OCCUPIED_FLAG_MASK } else { 0 } |
            (self.version & VERSION_MASK);
    }
}

#[must_use]
struct AllocError {
    message: String,
}

impl Debug for AllocError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("AllocError").finish()
    }
}

impl Display for AllocError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Allocation failed: {}", self.message)
    }
}

impl Error for AllocError {}

impl AllocError {
    fn new(message: impl Into<String>) -> Self {
        Self { message: message.into() }
    }
}

#[inline(always)]
fn next_aligned_value(base_val: usize, align: usize) -> usize {
    assert_ne!(align, 0, "Alignment must not be zero");
    // We create a bitmask from the alignment value by subtracting one
    // (e.g. 0x0010 -> 0x000F) and then inverting it (0x000F -> 0xFFF0). Then,
    // we AND it with the base address minus 1 to get the next aligned address
    // in the direction of zero, then add the alignment "chunk" size to get the
    // next aligned address in the direction of max size_t.
    //
    // Subtracting one from the base address accounts for the case where the
    // address is already aligned.
    ((base_val - 1) & !(align - 1)) + align
}
