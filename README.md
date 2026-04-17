# Route Optimization Service

## 🧩 Idea

A service for building an optimal route between points of interest (POI) considering:

* opening hours,
* user priorities,
* time constraints.

The system takes a set of points and conditions as input and returns:

* visit order,
* schedule,
* basic route metrics.

---

## ⚙️ Planned Architecture

* **Backend:** Java (Spring Boot)
* **Solver:** C++ (route optimization algorithm)
* **Communication:** JSON (backend calls C++ executable)

---

## 📌 Status

🚧 Work in progress (MVP stage)

---

## 📅 TODO

* [ ] Define input/output data format
* [ ] Implement basic C++ solver
* [ ] Set up Spring Boot API
* [ ] Connect backend with solver
* [ ] Add simple frontend
