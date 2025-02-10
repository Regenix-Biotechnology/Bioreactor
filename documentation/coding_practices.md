# Coding Guidelines

This document establishes standard programming rules to ensure consistency, improve code quality, and facilitate collaboration within the team.

## 1. Git Rules

- When fixing a bug, implementing a new feature, or making changes, **create a new branch** from the `main` branch.
- This helps avoid conflicts and keeps the `main` branch clean.
- After completing your work, **create a pull request (PR)** to merge your branch into `main`.
- At least **one team member must review your code** before merging.

## 2. Naming Conventions

Consistent naming makes the code easier to read and maintain. Follow these conventions:

- **camelCase** → For variables, functions, and methods.  
  _Example:_ `calculateSpeed()`, `totalAmount`
- **PascalCase** → For class names.  
  _Example:_ `UserProfile`, `DataProcessor`
- **UPPERCASE** → For constants.  
  _Example:_ `MAX_SPEED`, `API_KEY`
- **snake_case** → For file names.  
  _Example:_ `user_profile.py`, `data_processor.js`
- **Boolean variables** should start with `is`, `has`, `can`, etc.  
  _Example:_ `isAvailable`, `hasPermission`, `canExecute`

Following these rules will help maintain clarity and improve collaboration across the team.

## 3. Documentation

- When adding a class for a new sensor, add the link to the datasheet in the class documentation.
- **Document your code** using [Doxygen](https://www.doxygen.nl/) style comments to improve maintainability.  This will make those informations available when you hover over a function or class in VSCode.
- Use comments to describe the purpose and behavior of functions, parameters, and return values.

### Example:

```cpp
/**
 * @brief Calculates the speed of an object.
 * @param distance The distance traveled (in meters).
 * @param time The time taken (in seconds).
 * @return The calculated speed (in meters per second).
 */
double calculateSpeed(double distance, double time) {
    return distance / time;
}
```



This version keeps the document concise while improving readability. Let me know if you want to add anything else! 🚀
