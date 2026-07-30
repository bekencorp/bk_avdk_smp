## CMSIS Software Pack {#cmsis_pack}

The **CMSIS Base Components** are actively maintained in the [**CMSIS 6 GitHub repository**](https://github.com/ARM-software/CMSIS_6) and are released together in the [**CMSIS pack**](https://www.keil.arm.com/packs/cmsis-arm/) that follows the [Open-CMSIS-Pack format](https://open-cmsis-pack.github.io/Open-CMSIS-Pack-Spec/main/html/index.html).

The table below shows the high-level structure of the **ARM::CMSIS** pack. Details about component folders can be found in the referenced component documentations.

File/Directory        | Content
:---------------------|:-------------------
📂 CMSIS               | CMSIS Base software components folder
   ┣ 📂 Core           | Processor files for the [CMSIS-Core (Cortex-M)](../Core/index.html)
   ┣ 📂 Core_A         | Processor files for the [CMSIS-Core (Cortex-A)](../Core_A/index.html)
   ┣ 📂 Documentation  | A local copy of this documentation
   ┣ 📂 Driver         | API header files and template implementations for the [CMSIS-Driver](../Driver/index.html)
   ┗ 📂 RTOS2          | API header files and OS tick implementations for the [CMSIS-RTOS2](../RTOS2/index.html)
📄 ARM.CMSIS.pdsc      | Package description file in CMSIS-Pack format
📄 LICENSE             | CMSIS License Agreement (Apache 2.0)

Section \ref cmsis_components provides links to CMSIS packs and repositories of other CMSIS components that are delivered separately and are not part of ARM::CMSIS pack.