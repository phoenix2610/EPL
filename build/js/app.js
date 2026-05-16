        'use strict';

        /* ── Custom Cursor ── */
        (function initCursor() {
            const cur = document.getElementById('cur');
            const ring = document.getElementById('ring');

            let mx = -100, my = -100; // start off-screen
            let rx = -100, ry = -100;

            document.addEventListener('mousemove', e => {
                mx = e.clientX;
                my = e.clientY;
            });

            // Slightly lag the ring with RAF for smoothness
            function tick() {
                cur.style.left = mx + 'px';
                cur.style.top = my + 'px';

                rx += (mx - rx) * 0.18;
                ry += (my - ry) * 0.18;
                ring.style.left = rx + 'px';
                ring.style.top = ry + 'px';

                requestAnimationFrame(tick);
            }
            requestAnimationFrame(tick);

            // Enlarge ring on interactive elements
            const interactables = 'a, button, [role="button"]';
            document.querySelectorAll(interactables).forEach(el => {
                el.addEventListener('mouseenter', () => {
                    ring.style.transform = 'translate(-50%,-50%) scale(1.8)';
                    ring.style.borderColor = 'rgba(200,255,0,0.6)';
                });
                el.addEventListener('mouseleave', () => {
                    ring.style.transform = 'translate(-50%,-50%) scale(1)';
                    ring.style.borderColor = 'rgba(255,255,255,0.4)';
                });
            });
        })();


        /* ── Animated Counter ── */
        function animateCounter(el, target, duration, suffix) {
            suffix = suffix || '';
            const start = performance.now();
            const startVal = 0;

            function update(now) {
                const elapsed = now - start;
                const progress = Math.min(elapsed / duration, 1);
                // Ease out cubic
                const eased = 1 - Math.pow(1 - progress, 3);
                const current = Math.round(startVal + eased * (target - startVal));

                el.textContent = current.toLocaleString() + suffix;

                if (progress < 1) requestAnimationFrame(update);
            }
            requestAnimationFrame(update);
        }

        // Trigger counters when stats strip enters viewport
        (function initCounters() {
            const locEl = document.getElementById('stat-loc');
            const devEl = document.getElementById('stat-devs');
            let fired = false;

            const observer = new IntersectionObserver(entries => {
                entries.forEach(entry => {
                    if (entry.isIntersecting && !fired) {
                        fired = true;
                        animateCounter(locEl, 28000, 1800);
                        animateCounter(devEl, 847, 1400);
                        observer.disconnect();
                    }
                });
            }, { threshold: 0.4 });

            observer.observe(document.querySelector('.stats-strip'));
        })();


        /* ── Marquee / Ticker ── */
        (function initMarquee() {
            const items = [
                'SEMANTIC COMPILER',
                'EPF WEB ENGINE',
                'EPD PROTOCOL',
                '3D ORBITAL ENGINE',
                'SMART COLLECTIONS',
                'SMART FILTER',
                'BYTECODE VM — COMING v2.0',
                'LLVM NATIVE — COMING v3.0',
                'ZERO IMPORTS',
                'PLAIN ENGLISH',
                '28K LINES OF EPL',
                '847 EARLY DEVS',
            ];

            const track = document.getElementById('marquee');

            // Duplicate for seamless loop
            const doubled = [...items, ...items];
            doubled.forEach(text => {
                const div = document.createElement('div');
                div.className = 'marquee-item';
                div.innerHTML = `<span aria-hidden="true">◆</span>${text}`;
                track.appendChild(div);
            });

            // Calculate one full set width and set animation duration dynamically
            // (CSS handles the animation; we just need the content)
        })();


        /* ── Scroll Reveal ── */
        (function initReveal() {
            const els = document.querySelectorAll('.reveal');

            const observer = new IntersectionObserver(entries => {
                entries.forEach((entry, i) => {
                    if (entry.isIntersecting) {
                        // Stagger cards in the same grid
                        const delay = entry.target.closest('.features-grid')
                            ? Array.from(entry.target.parentElement.children).indexOf(entry.target) * 80
                            : 0;

                        setTimeout(() => {
                            entry.target.classList.add('visible');
                        }, delay);

                        observer.unobserve(entry.target);
                    }
                });
            }, { threshold: 0.15 });

            els.forEach(el => observer.observe(el));
        })();


        /* ── Nav: highlight current page ── */
        (function highlightNav() {
            const links = document.querySelectorAll('.nav-links a');
            const path = window.location.pathname.split('/').pop() || 'index.html';

            links.forEach(link => {
                if (link.getAttribute('href') === path) {
                    link.style.color = '#fff';
                    link.setAttribute('aria-current', 'page');
                }
            });
        })();


        /* ── Nav: shrink and logo rotation on scroll ── */
        (function initNavScroll() {
            const nav = document.querySelector('.nav');
            const logoImg = document.querySelector('.logo-img');
            window.addEventListener('scroll', () => {
                if (window.scrollY > 60) {
                    nav.style.padding = '1rem 3rem';
                } else {
                    nav.style.padding = '1.5rem 3rem';
                }
                
                // Spin like a coin
                if (logoImg) {
                    const rot = window.scrollY * 0.5;
                    logoImg.style.transform = `rotateY(${rot}deg)`;
                }
            }, { passive: true });
        })();
