# Transaction behaviour
Transaction behaviour in relation to system state.

## Process creation
A process created inside a transaction will be destroyed if the transaction is rolled back, and will be made visible to the system on commit. Any operations on the process object before it has been comitted must be made using the same transaction handle the process creation is recorded in.

## Nested transactions
If a transaction is created inside another transaction committing the child transaction only commits to the parent transaction, changes are not made visible to the system until the top transaction is comitted.

## Handle orphaning
Any handle created inside a transaction will be orphaned and cannot be used if the transaction is rolled back.