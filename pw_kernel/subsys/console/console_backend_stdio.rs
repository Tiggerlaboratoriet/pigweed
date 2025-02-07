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
use pw_status::{Error, Result};
use std::io::{stdout, Write as StdWrite};

#[no_mangle]
pub fn console_backend_write(buf: &[u8]) -> Result<usize> {
    stdout().lock().write(buf).map_err(|_| Error::Unknown)
}

#[no_mangle]
pub fn console_backend_flush() -> Result<()> {
    stdout().lock().flush().map_err(|_| Error::Unknown)
}
