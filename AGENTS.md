# Performance reporting

- Use simple Markdown tables with rows for 10k, 100k, and issue48.
- Diagram generation: `Case | Dev | Current | Boost | Current/Boost`.
- Unique-vertex gathering: `Case | Dev | Current | Boost`; exclude diagram generation and output allocation from the timing.
- Memory: `Case | Dev | Current | Boost | Current/Boost`; include peak bytes and allocation counts.
- Keep diagram generation, unique-vertex gathering, and memory in separate tables.
