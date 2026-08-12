-- Схема бази даних для системи управління нарядами
-- Включає таблиці для особового складу, звань, типів нарядів та графіка

-- Таблиця звань
CREATE TABLE IF NOT EXISTS ranks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    priority INTEGER DEFAULT 0
);

-- Таблиця особового складу
CREATE TABLE IF NOT EXISTS personnel (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rank_id INTEGER,
    name TEXT NOT NULL,
    position TEXT,
    notes TEXT DEFAULT 'в наявності',
    FOREIGN KEY(rank_id) REFERENCES ranks(id)
);

-- Таблиця видів нарядів
CREATE TABLE IF NOT EXISTS duty_types (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    person_count INTEGER DEFAULT 1,
    min_rank_id INTEGER,
    max_rank_id INTEGER,
    rest_days INTEGER DEFAULT 1,
    description TEXT,
    FOREIGN KEY(min_rank_id) REFERENCES ranks(id),
    FOREIGN KEY(max_rank_id) REFERENCES ranks(id)
);

-- Таблиця графіка нарядів
CREATE TABLE IF NOT EXISTS schedule (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    duty_date DATE NOT NULL,
    person_id INTEGER,
    duty_type_id INTEGER,
    is_manual INTEGER DEFAULT 0, -- 0 для автоматичного, 1 для ручного
    FOREIGN KEY(person_id) REFERENCES personnel(id),
    FOREIGN KEY(duty_type_id) REFERENCES duty_types(id)
);

-- Таблиця статусів особового складу (відпустки, лікарняні тощо)
CREATE TABLE IF NOT EXISTS personnel_statuses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    person_id INTEGER,
    status_name TEXT, 
    start_date TEXT,
    end_date TEXT,
    notes TEXT,
    FOREIGN KEY(person_id) REFERENCES personnel(id) ON DELETE CASCADE
);

-- Початкові дані звань
INSERT OR IGNORE INTO ranks (name, priority) VALUES 
('Солдат', 1), ('Старший солдат', 2), ('Молодший сержант', 3), ('Сержант', 4), 
('Старший сержант', 5), ('Головний сержант', 6), ('Штаб-сержант', 7), 
('Майстер-сержант', 8), ('Старший майстер-сержант', 9), ('Головний майстер-сержант', 10),
('Молодший лейтенант', 11), ('Лейтенант', 12), ('Старший лейтенант', 13), 
('Капітан', 14), ('Майор', 15), ('Підполковник', 16), ('Полковник', 17);
