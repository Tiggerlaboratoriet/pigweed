// Copyright 2025 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#![no_std]
#![no_main]
use spinlock::*;
use unittest::test;

#[test]
fn bare_try_lock_returns_correct_value() -> unittest::Result<()> {
    let lock = BareSpinLock::new();

    {
        let _sentinel = lock.lock();
        unittest::assert_true!(lock.try_lock().is_none());
    }

    unittest::assert_true!(lock.try_lock().is_some());

    Ok(())
}

#[test]
fn try_lock_returns_correct_value() -> unittest::Result<()> {
    let lock = SpinLock::new(false);

    {
        let mut guard = lock.lock();
        *guard = true;
        unittest::assert_true!(lock.try_lock().is_none());
    }

    let guard = lock.lock();
    unittest::assert_true!(*guard);

    Ok(())
}
