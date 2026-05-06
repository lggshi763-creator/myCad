myCad API Reference
===================

.. note::
   The C++ symbol pages on this site are generated from Doxygen XML at build
   time via the `Breathe`_ Sphinx extension.  The structured **brief** of every
   entry is in English; **detailed descriptions** may be in Chinese.

.. _Breathe: https://breathe.readthedocs.io/

This site is part of the myCad documentation set:

* **Architecture & roadmap**: ``docs/architecture/`` — design rationale, layering,
  paradigm choices, sprint plan.
* **ADRs**: ``docs/adr/`` — recorded technical decisions.
* **API reference** *(this site)* — every documented public symbol in
  ``src/{domain,application,infrastructure,plugin,ui}/``.

How to read this reference
--------------------------

The codebase is organized in four layers; each gets its own page:

.. toctree::
   :maxdepth: 2
   :caption: Layers

   layers/domain
   layers/application
   layers/infrastructure
   layers/ui

Each layer page lists the namespaces, classes, and free functions that have
Doxygen comments. Undocumented symbols are intentionally hidden — see the
contributor guide for the comment style.

Indices
-------

* :ref:`genindex`
* :ref:`search`
