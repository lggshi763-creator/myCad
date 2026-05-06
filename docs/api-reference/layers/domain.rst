Domain layer
============

The domain layer holds aggregates, value objects, and domain events.
It has **zero external dependencies** (per ADR-0002) — no Qt, no OCCT,
no fmt — only the C++20 standard library.

.. note::
   Anything below appears here because someone wrote a Doxygen comment for it
   in the source.  If you don't see a class you expect, the symbol just isn't
   documented yet.

Namespace ``mycad::domain``
---------------------------

.. doxygennamespace:: mycad::domain
   :members:
   :undoc-members:
