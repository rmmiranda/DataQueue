# DataQueue

The repository for the database model used in implementing the message log of TR's M7 product. 
 
The DataQueue version will be controlled from this repo. View this README online at https://bitbucket.org/swiftlabsinc/data-queue/src/master/README.md

## Checkout

```
git clone https://<user>@bitbucket.org/swiftlabsinc/data-queue.git DataQueue
cd DataQueue
```

## Development Environment

### File Structure

#### src

The platform software (or OS) independent and hardware independent source code.

#### inc

The header files required by the source code and exported to the platform specific source code.

#### psl

The Platform Software Layer (PSL) which contains source code specific to the platform software.

#### fsal

The Filesystem Abstraction Layer (FSAL) which contins source code specific to the file system.

## Build

```
cd DataQueue
./build_data_queue.sh
```

This repository is not automatically built.

## Test

*Commands to run tests to be filled in by developers*

## Publish

*Steps to publish products to be filled in by developers.  Typically, you simply need to increment a version number.*
*If the instructions are lengthy or differ substantially by product, place in separate file(s) and reference here.*

## License

© TR


