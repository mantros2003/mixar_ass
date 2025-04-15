# Node based GUI image manipulation tool

## Project Overview

Node-based interface for image manipulation in C++. This application allows users to load images, process them through a series of connected nodes, and save the resulting output.

**Key Highlights:**

* Operations implemented as nodes
* Intuitive GUI

## Features Implemented

This project currently implements the following features:

* **Blur Image**
* **Change Brightness**
* **Save Image**

## Build Instructions

Follow these instructions to build and run the project on your local machine.

### Prerequisites

Before you begin, ensure you have the following installed:

* **[opencv]:**
    ```bash
    brew install opencv
    # Or equivalent commands for other platforms
    ```
### Cloning the Repository

1.  Clone the repository to your local machine:
    ```bash
    git clone https://github.com/mantros2003/mixar_ass.git
    cd mixar_ass
    ```

### Building the Project
#### Using Make

1.  Navigate to the project directory:
    ```bash
    cd [Your Repository Name]
    ```

2.  Edit Makefile and change "OPENCVINCLUDEPATH" to the location where opencv headers are stored, "OPENCVLIBPATH" to where opencv dynamic libraries are stored. For opencv installed on MacOs using Homebrew:
    ```Makefile
    # Example
    OPENCVINCLUDEPATH = /opt/homebrew/opt/opencv/include/opencv4
    OPENCVLIBPATH = /opt/homebrew/opt/opencv/lib
    ```

4.  Build the project using the `make` command:
    ```bash
    make
    ```

### Running the Application

Once the project is built successfully, you can run the application using the following command:

```bash
./app
