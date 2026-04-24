-- Схема бази даних для модуля «Графік нарядів ВЧ»

-- Звання
CREATE TABLE IF NOT EXISTS ranks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE, -- напр. "Лейтенант", "Капітан"
    priority INTEGER DEFAULT 0  -- для сортування за старшинством
);

-- Особовий склад
CREATE TABLE IF NOT EXISTS personnel (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rank_id INTEGER,
    full_name TEXT NOT NULL,
    position TEXT,
    is_active INTEGER DEFAULT 1, -- чи служить ще в частині
    FOREIGN KEY (rank_id) REFERENCES ranks(id)
);

-- Види нарядів
CREATE TABLE IF NOT EXISTS duty_types (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE, -- напр. "Черговий частини", "Варта"
    abbr TEXT,                 -- абревіатура для сітки
    min_rank_id INTEGER,       -- мінімальне звання для цього наряду
    color_code TEXT,           -- колір для відображення в інтерфейсі
    FOREIGN KEY (min_rank_id) REFERENCES ranks(id)
);

-- Допуски особового складу до конкретних нарядів
CREATE TABLE IF NOT EXISTS duty_clearances (
    person_id INTEGER,
    duty_type_id INTEGER,
    PRIMARY KEY (person_id, duty_type_id),
    FOREIGN KEY (person_id) REFERENCES personnel(id),
    FOREIGN KEY (duty_type_id) REFERENCES duty_types(id)
);

-- Статуси особового складу (відпустки, лікарняні, відрядження)
CREATE TABLE IF NOT EXISTS personnel_statuses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    person_id INTEGER,
    status_type TEXT CHECK(status_type IN ('Відпустка', 'Лікарняний', 'Відрядження', 'Інше')),
    start_date DATE NOT NULL,
    end_date DATE NOT NULL,
    comment TEXT,
    FOREIGN KEY (person_id) REFERENCES personnel(id)
);

-- Графік нарядів (фінальна сітка)
CREATE TABLE IF NOT EXISTS schedule (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    duty_date DATE NOT NULL,
    person_id INTEGER,
    duty_type_id INTEGER,
    is_manual INTEGER DEFAULT 0, -- чи було внесено правку вручну
    FOREIGN KEY (person_id) REFERENCES personnel(id),
    FOREIGN KEY (duty_type_id) REFERENCES duty_types(id)
);

-- Логування змін (безпека)
CREATE TABLE IF NOT EXISTS audit_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    user_action TEXT NOT NULL,
    details TEXT
);

-- Початкові дані: Звання
INSERT OR IGNORE INTO ranks (name, priority) VALUES 
('Солдат', 1), ('Старший солдат', 2), ('Молодший сержант', 3), ('Сержант', 4), 
('Старший сержант', 5), ('Головний сержант', 6), ('Штаб-сержант', 7), ('Молодший лейтенант', 10), 
('Лейтенант', 11), ('Старший лейтенант', 12), ('Капітан', 13), ('Майор', 14), 
('Підполковник', 15), ('Полковник', 16);
